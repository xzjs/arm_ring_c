import glob
import json
import os
import socket
import struct
import threading
import time
import csv
import numpy as np
import pandas as pd
import serial
from datetime import datetime
from sklearn.metrics.pairwise import cosine_similarity

action_dict = {0: "伸五指",
               1: "手腕内旋",
               2: "手腕右摆",
               3: "手腕外旋",
               4: "手腕左摆",
               5: "握拳"}
frame_header = bytearray(b'\xAA\x55')  # 帧头
frame_footer = b'\x0D'  # 帧尾
# 动作校准相关参数
window_size_train = 40
rate_train = 0.2
sum_data_avg_threshold = 5000
sum_data_len = 50

# 动作测试相关参数
window_size_test = 20
rate_test = 0.2

stop_debug_phase = False


def stop_debug_phase_thread():
    global stop_debug_phase
    stop_debug_phase = True


def debug_phase():
    received_data = []
    sum_data = []
    global stop_debug_phase
    while not stop_debug_phase:
        # while True:
        received_bytes = ser.read(40)
        if received_bytes[0:2] != frame_header[:] or received_bytes[-1] != frame_footer[0]:
            while True:
                temp_byte = ser.read(1)
                if temp_byte[0] == frame_header[0]:
                    temp_byte = ser.read(1)
                    if temp_byte[0] == frame_header[1]:
                        temp_byte = ser.read(38)
                        if temp_byte[-1] == frame_footer[0]:
                            break
            continue

        # 处理收到的数据
        data_bytes = received_bytes
        # 提取数据字节并转化为十进制
        data_groups = list(struct.unpack('<7i', data_bytes[4:32]))

        sum_data_groups = sum(data_groups)
        sum_data.append(sum_data_groups)  # 1
        received_data.append(data_groups)  # 50 * 7
        if len(sum_data) == sum_data_len:
            sum_data_avg = sum(sum_data) / len(sum_data)  # 标量1
            received_data_avg = np.mean(received_data, axis=0)  # 1 * 7
            if sum_data_avg > sum_data_avg_threshold:
                str1 = f"{received_data_avg[0]},{received_data_avg[1]},{received_data_avg[2]},{received_data_avg[3]},{received_data_avg[4]},{received_data_avg[5]},{received_data_avg[6]}\n"
                udp.sendto(bytearray(str1, encoding="utf-8"), address)
            else:
                received_data_avg = [0, 0, 0, 0, 0, 0, 0]
                str1 = f"{received_data_avg[0]},{received_data_avg[1]},{received_data_avg[2]},{received_data_avg[3]},{received_data_avg[4]},{received_data_avg[5]},{received_data_avg[6]}\n"
                udp.sendto(bytearray(str1, encoding="utf-8"), address)
            sum_data = []
            received_data = []


def send_at_command(ser, command):
    ser.write(command.encode())
    time.sleep(1)  # 等待一段时间以确保蓝牙模块有足够的时间处理指令
    response = ser.read_all()
    print(response)
    return response


#  动作校准函数
def action_calibration(label, folder_name_train):
    received_data = []  # 处理接受到的数据
    sum_data = []
    received_used = []  # 真正实用的数据
    sum_used = []

    # filename = os.path.join(folder_name_train, f'{label}.csv')
    filename_res = os.path.join(folder_name_train, 'res.json')

    start = None
    waiting = True
    while True:
        received_bytes = ser.read(40)
        if received_bytes[0:2] != frame_header[:] or received_bytes[-1] != frame_footer[0]:
            while True:
                temp_byte = ser.read(1)
                if temp_byte[0] == frame_header[0]:
                    temp_byte = ser.read(1)
                    if temp_byte[0] == frame_header[1]:
                        temp_byte = ser.read(38)
                        if temp_byte[-1] == frame_footer[0]:
                            break
            continue
        # 记录开始时间
        start = start if start else time.time() * 1000
        # 处理收到的数据
        data_bytes = received_bytes
        # 取出一组向量
        data_groups = list(struct.unpack('<7i', data_bytes[4:32]))
        current = time.time() * 1000
        time_diff = current - start
        if waiting:
            if time_diff >= 1000:
                print('over 1s...')
                waiting = False
            continue
        # 把一组向量求和
        sum_data_groups = sum(data_groups)
        # 50组向量和
        sum_data.append(sum_data_groups)  # 1
        received_data.append(data_groups)  # 50 * 7
        # 50条数据处理压缩成一条
        if len(sum_data) == sum_data_len:
            sum_data_avg = sum(sum_data) / len(sum_data)  # 标量1

            received_data_avg = np.mean(received_data, axis=0)  # 1 * 7
            print(received_data_avg)
            if sum_data_avg > sum_data_avg_threshold:
                
                
                received_used.append(received_data_avg)
                sum_used.append(sum_data_avg)
            else:
                received_data_avg = [0, 0, 0, 0, 0, 0, 0]
                
                received_used.append(received_data_avg)
                sum_used.append(0)
            sum_data = []
            received_data = []
        # 可用数据超过40行
        if len(sum_used) >= window_size_train:
            sum_used_window = sum_used[-window_size_train:]
            count = sum(1 for data in sum_used_window if data == 0)
            print(f'0行：【{count}】,规定：【{int(window_size_train * rate_train)}】')
            # 空行小于20%
            if count <= window_size_train * rate_train:
                received_used_window = np.array(received_used[-window_size_train:])
                print(f'all: {len(received_used_window)}')
                non_zero_mask = (received_used_window != 0).any(axis=1)
                non_zero_columns = received_used_window[non_zero_mask]
                print(f'非0:{len(non_zero_columns)}')
                average = np.mean(non_zero_columns, axis=0, keepdims=False)
                if not os.path.exists(filename_res):
                    data = {}
                else:
                    with open(filename_res, 'r') as json_file:
                        data = json.load(json_file)
                print(average)
                data[str(label)] = list(average)
                with open(filename_res, 'w') as json_file:
                    json.dump(data, json_file)
                return
    udp.close()


# 进入校准环节
def calibration_process(folder_name_train):
    while True:
        # 打印指令菜单
        print("可用指令：")
        for key, value in action_dict.items():
            print(f"{key}: {value}")

        input_str = input("请输入要执行的指令代码或输入 'q' 退出：")

        if input_str == "q":
            break
        try:
            input_int = int(input_str)
            if input_int in action_dict:
                action_name = action_dict[input_int]
                print(f"开始校准动作【{action_name}】")
                action_calibration(input_int, folder_name_train)
                print(f"动作【{action_name}】校准完成")
            else:
                print("请输入正确的指令代码！")
        except ValueError:
            print("请输入正确的指令代码！")


# 进入动作测试环节
def action_test():
    start = None
    waiting = True

    max_similarity = -1  # 初始化为一个负数
    most_similar_k = None

    received_data = []  # 处理接受到的数据
    sum_data = []
    received_used = []  # 真正实用的数据
    sum_used = []
    all_data = []

    current_directory = os.path.dirname(os.path.abspath(__file__))  # 获取当前目录
    subfolders = glob.glob(os.path.join(current_directory, "train_*"))  # 找到训练文件夹[xx, xx]
    if subfolders:
        latest_folder = max(subfolders, key=os.path.basename)
        if len(os.listdir(latest_folder)) == 0:
            latest_folder = subfolders[-2]
        else:
            latest_folder = subfolders[-1]
    else:
        print("未采集数据！")

    filename = 'x.csv'
    with open(filename, 'w') as file:
        pass

    res_json = os.path.join(latest_folder, 'res.json')
    with open(res_json, 'r') as json_file:
        data = json.load(json_file)

    while True:
        received_bytes = ser.read(40)
        if received_bytes[0:2] != frame_header[:] or received_bytes[-1] != frame_footer[0]:
            while True:
                temp_byte = ser.read(1)
                if temp_byte[0] == frame_header[0]:
                    temp_byte = ser.read(1)
                    if temp_byte[0] == frame_header[1]:
                        temp_byte = ser.read(38)
                        if temp_byte[-1] == frame_footer[0]:
                            break
            continue

        start = start if start else time.time() * 1000
        # 处理收到的数据
        data_bytes = received_bytes
        # 提取数据字节并转化为十进制
        data_groups = list(struct.unpack('<7i', data_bytes[4:32]))
        current = time.time() * 1000
        time_diff = current - start
        if waiting:
            if time_diff >= 1000:
                print('over 1s...')
                waiting = False
            continue
        all_data.append(data_groups)
        print(f'{all_data[-1]}, 采样次数【{len(all_data)}】')

        sum_data_groups = sum(data_groups)
        sum_data.append(sum_data_groups)  # 1
        received_data.append(data_groups)  # 50 * 7
        if len(sum_data) == sum_data_len:
            sum_data_avg = sum(sum_data) / len(sum_data)  # 标量1
            received_data_avg = np.mean(received_data, axis=0)  # 1 * 7
            if sum_data_avg > sum_data_avg_threshold:
                str1 = f"{received_data_avg[0]},{received_data_avg[1]},{received_data_avg[2]},{received_data_avg[3]},{received_data_avg[4]},{received_data_avg[5]},{received_data_avg[6]}\n"
                udp.sendto(bytearray(str1, encoding="utf-8"), address)
                received_used.append(received_data_avg)
                sum_used.append(sum_data_avg)
            else:
                received_data_avg = [0, 0, 0, 0, 0, 0, 0]
                str1 = f"{received_data_avg[0]},{received_data_avg[1]},{received_data_avg[2]},{received_data_avg[3]},{received_data_avg[4]},{received_data_avg[5]},{received_data_avg[6]}\n"
                udp.sendto(bytearray(str1, encoding="utf-8"), address)
                received_used.append(received_data_avg)
                sum_used.append(0)
            sum_data = []
            received_data = []
        if len(sum_used) >= window_size_test:
            sum_used_window = sum_used[-window_size_test:]
            count = sum(1 for data1 in sum_used_window if data1 == 0)
            print(f'0行：【{count}】,规定：【{int(window_size_test * rate_test)}】')
            if count <= window_size_test * rate_test:
                received_used_window = np.array(received_used[-window_size_test:])
                # print(received_used_window)
                print(f'all: {len(received_used_window)}')
                non_zero_mask = (received_used_window != 0).any(axis=1)
                non_zero_columns = received_used_window[non_zero_mask]
                # print(non_zero_columns)
                print(f'非0:{len(non_zero_columns)}')
                test = np.array(non_zero_columns)
                # print(data)
                for k, v in data.items():
                    # print(f'v:{v}, {type(v)}')
                    train = np.array(v).reshape(1, -1)
                    # print(test)
                    similarity1 = cosine_similarity(test, train)
                    # print(similarity1)
                    similarity1 = np.sort(similarity1, axis=0)
                    similarity2 = similarity1[3: -3]
                    # print(similarity2)
                    similarity = np.mean(similarity2)
                    print(f'{action_dict[int(k)]} : 【{similarity}】')
                    # 如果余弦相似度更大，则更新最大相似度和最相似的k
                    if similarity > max_similarity:
                        max_similarity = similarity
                        most_similar_k = k
                print('**********************************************')
                print("最相似的动作:", action_dict[int(most_similar_k)])
                print("最大余弦相似度:", max_similarity)
                print('**********************************************')
                return
    udp.close()


# 启动交互脚本
def start_ui():
    debug_phase_thread = threading.Thread(target=debug_phase)
    debug_phase_thread.start()

    current_time = datetime.now().strftime("%Y-%m-%d %H-%M-%S")
    current_time_train = f'train_{current_time}'
    folder_name_train = current_time_train.replace(":", "_")  # 将冒号替换为下划线
    os.makedirs(folder_name_train, exist_ok=True)
    print(f'目录{folder_name_train}创建成功')
    introducer = "请输入指令代码：1-进入校准环节；2-进入动作测试环节；3-退出程序\n"
    while True:
        input_str = input(introducer)
        if input_str == "3":
            stop_debug_phase_thread()
            break
        elif input_str == "1":
            stop_debug_phase_thread()
            print("您已进入校准环节，请根据指令做动作！")
            calibration_process(folder_name_train)
        elif input_str == "2":
            stop_debug_phase_thread()
            # time.sleep(1)
            print("您已进入测试环节！")
            action_test()
        else:
            # stop_debug_phase_thread()
            print("请输入正确的指令代码！")


if __name__ == "__main__":
    port = 'COM3'  # 串口号
    baudrate = 1000000  # 波特率
    timeout = 5  # 超时时间
    stopbits = serial.STOPBITS_ONE  # 停止位设置为1
    bytesize = serial.EIGHTBITS  # 数据位设置为8
    parity = serial.PARITY_NONE  # 校验位设置为None
    # 创建串口对象，并设置串口参数
    ser = serial.Serial(port, baudrate, timeout=timeout, stopbits=stopbits, bytesize=bytesize, parity=parity)
    # 初始化配置
    send_at_command(ser, "+++a")  # 进入AT模式
    send_at_command(ser, "AT+MODE=M\r\n")  # 设置模块为透传模式
    send_at_command(ser, "+++a")
    send_at_command(ser, "AT+WMODE=0\r\n")  # 设置工作模式
    send_at_command(ser, "AT+SCAN\r\n")  # 开始扫描
    send_at_command(ser, "AT+CONN=D4AD20575264\r\n")  # 连接指定设备
    send_at_command(ser, "AT+LINK\r\n")  # 建立连接
    send_at_command(ser, "AT+ENTM\r\n")  # 进入透传模式
    print('初始化结束......')

    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
    address = ("127.0.0.1", 1347)

    start_ui()
