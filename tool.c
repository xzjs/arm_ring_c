#include <stdio.h>
#include <math.h>
#include "tool.h"

/**
 * np.mean
 * array 矩阵结构体
 */
MATRIX mean(MATRIX array)
{
    static double avg[COL] = {0};
    for (int j = 0; j < array.col; j++)
    {
        double sum = 0;
        for (int i = 0; i < array.row; i++)
        {
            sum += array.array[i * array.col + j];
        }
        avg[j] = sum / array.row;
    }
    MATRIX m =
        {
            avg,
            1,
            COL};
    return m;
}

/**
 * 求每个向量和，再拿和求平均
 */
double sum_average(MATRIX m)
{
    double sum = 0;
    for (int i = 0; i < m.col * m.row; i++)
    {
        sum += m.array[i];
    }
    return sum / m.row;
}

/**
 * 处理数据
 * m 1000*7的矩阵
 * 将1000*7的向量50一组压缩成20*7的向量
 * 判断最后剩余的向量是否低于阈值
 * 若低于阈值，返回的结构体row为0
 * 否则即为可用的矩阵
 */
MATRIX processing_data(double *arr)
{
    int index = 0;
    double temp[SUM_DATA_LEN][COL];
    double result_array[WINDOW_SIZE_TRAIN][COL];
    MATRIX result = {
        (double *)result_array,
        0,
        COL};
    while (index < TOTAL)
    {
        double sum = 0;
        for (int i = 0; i < SUM_DATA_LEN; i++)
        {
            for (int j = 0; j < COL; j++)
            {
                temp[i][j] = arr[index];
                sum += arr[index++];
            }
        }
        double avg = sum / SUM_DATA_LEN;
        if (avg > SUM_DATA_AVG_THRESHOLD)
        {
            MATRIX m_50_7 = {(double *)temp, SUM_DATA_LEN, COL};
            MATRIX m_1_7 = mean(m_50_7);
            for (int j = 0; j < COL; j++)
            {
                result.array[result.row * COL + j] = m_1_7.array[j];
            }
            result.row++;
        }
    }
    if (result.row < WINDOW_SIZE_TRAIN * RATE_TRAIN)
    {
        result.row = 0;
    }
    return result;
}

/**
 * 循环更新DATA数组
 */
void update_data(int received_data[COL])
{
    for (int i = 0; i < COL; i++)
    {
        DATA.arr[DATA.index][i] = (double)received_data[i];
    }
    if (DATA.row < ROW)
    {
        DATA.row++;
    }
    DATA.index++;
    if (DATA.index == ROW)
    {
        DATA.index = 0;
    }
}

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

SIMILARITY action_test(double res[][COL], int received_data[COL])
{
    SIMILARITY similarity = {-1, -1};
    update_data(received_data);
    if (DATA.row < ROW)
    {
        return similarity;
    }
    MATRIX m = processing_data((double *)DATA.arr);
    if (m.row == 0)
    {
        return similarity;
    }
    MATRIX test = mean(m);
    double sim[6];
    int max = 0;
    for (int i = 0; i < 6; i++)
    {
        double res1[COL];
        for (int j = 0; j < COL; j++)
        {
            res1[j] = res[i][j];
        }
        sim[i] = cosine_similarity(res1, test.array, COL);
        if (sim[i] > sim[max])
        {
            max = i;
        }
    }
    similarity.action_id = max;
    similarity.max_similarity = sim[max];
    return similarity;
}
