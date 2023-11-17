#include <stdio.h>
#include <math.h>
#include "tool.h"

double temp[COL] = {0.0};            // 临时压缩数组
int input_index = 0;                 // 输入数组的索引
double m_50[WINDOW_SIZE_TRAIN][COL]; // 存储压缩后的向量
int m_50_size = 0;                   // 存储压缩后的向量的大小
int m_50_index = 0;                  // 存储压缩后的向量的索引

// 计算向量的模
double vector_magnitude(double *vector, int size)
{
    double sum = 0.0;
    for (int i = 0; i < size; i++)
    {
        sum += vector[i] * vector[i];
    }
    return sqrt(sum);
}

// 计算两个向量的点积
double dot_product(double *vector1, double *vector2, int size)
{
    double product = 0.0;
    for (int i = 0; i < size; i++)
    {
        product += vector1[i] * vector2[i];
    }
    return product;
}

// 计算余弦相似度
double cosine_similarity(double *vector1, double *vector2, int size)
{
    double magnitude1 = vector_magnitude(vector1, size);
    double magnitude2 = vector_magnitude(vector2, size);

    if (magnitude1 == 0.0 || magnitude2 == 0.0)
    {
        // 避免除以零
        return 0.0;
    }

    double dot = dot_product(vector1, vector2, size);
    return dot / (magnitude1 * magnitude2);
}

// 二维数组竖向求平均，返回一维数组
double *vertical_average(double *arr, int row, int col)
{
    static double result[COL];
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            result[j] += arr[i * col + j] / row;
        }
    }
    return result;
}

// 一维数组求和
double sum(double *arr, int size)
{
    double sum = 0.0;
    for (int i = 0; i < size; i++)
    {
        sum += arr[i];
    }
    return sum;
}

// 统计二维数组的非零行
int count_non_zero_row(double *arr, int row, int col)
{
    int count = 0;
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            if (arr[i * col + j] != 0.0)
            {
                count++;
                break;
            }
        }
    }
    return count;
}

/**
 * @brief 动作测试函数
 *
 * @param res 输入的矩阵数据
 * @param received_data 接收到的数据
 * @return SIMILARITY 返回动作相似度
 */
SIMILARITY action_test(double res[][COL], int received_data[COL])
{
    SIMILARITY similarity = {-1, -1}; // 初始化相似度为{-1, -1}
    // 存储输入数据,只存储50条数据
    for (int i = 0; i < COL; i++)
    {
        temp[i] += received_data[i];
    }
    if (++input_index == SUM_DATA_LEN)
    {
        input_index = 0;
    }
    else
    {
        return similarity;
    }

    // 压缩数据并存储
    for (int i = 0; i < COL; i++)
    {
        temp[i] /= SUM_DATA_LEN;
    }
    double temp_sum = sum(temp, COL);
    for (int i = 0; i < COL; i++)
    {
        if (temp_sum >= SUM_DATA_AVG_THRESHOLD)
        {
            m_50[m_50_index][i] = temp[i];
        }
        else
        {
            m_50[m_50_index][i] = 0;
        }
        temp[i] = 0; // 清空临时数组
    }
    if (++m_50_index == WINDOW_SIZE_TRAIN) // 循环索引
    {
        m_50_index = 0;
    }
    if (++m_50_size < WINDOW_SIZE_TRAIN) // 数据长度不足20条,就返回默认相似度
    {
        return similarity;
    }

    // 非零行数判断
    int count_non_zero = count_non_zero_row((double *)m_50, WINDOW_SIZE_TRAIN, COL);
    if (count_non_zero < WINDOW_SIZE_TRAIN * RATE_TRAIN) // 非零行数不足20%，就返回默认相似度
    {
        return similarity;
    }

    // 计算相似度
    double *temp1 = vertical_average((double *)m_50, WINDOW_SIZE_TRAIN, COL);
    double sim[6]; // 定义相似度数组
    int max = 0;   // 最大相似度的索引
    for (int i = 0; i < 6; i++)
    {
        double res1[COL]; // 临时矩阵
        for (int j = 0; j < COL; j++)
        {
            res1[j] = res[i][j]; // 将输入矩阵的每一行复制到临时矩阵中
        }
        sim[i] = cosine_similarity(res1, temp1, COL); // 计算与临时矩阵的余弦相似度
        if (sim[i] > sim[max])
        {
            max = i; // 如果当前相似度大于最大相似度，则更新最大相似度的索引
        }
    }
    similarity.action_id = max;           // 将最大相似度的索引赋值给动作ID
    similarity.max_similarity = sim[max]; // 将最大相似度赋值给最大相似度值
    m_50_size = 0;                        // 清空m_50数组
    return similarity;                    // 返回相似度
}
