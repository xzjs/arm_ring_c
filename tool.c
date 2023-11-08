#include <stdio.h>
#include <math.h>

const int SUM_DATA_LEN = 50;
const int COL = 8;
const int SUM_DATA_AVG_THRESHOLD = 5000;
const int WINDOW_SIZE_TRAIN = 40;
const double RATE_TRAIN = 0.8;
const int TOTAL = 2000;

typedef struct matrix
{
    double *array;
    int row;
    int col;
} MATRIX; // 矩阵

typedef struct similarity
{
    int action_id;         // 动作id
    double max_similarity; // 最大相似度
} SIMILARITY;              // 相似度

struct data
{
    int row;                // 当前行数
    int index;              // 更新数据的索引
    double arr[TOTAL][COL]; // 存储数据的数组
} DATA;                     // 存储接收到的数据

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
            sum += array.array[i * array.row + j];
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
 * m 2000*7的矩阵
 * 将2000*7的向量50一组压缩成40*7的向量
 * 判断最后剩余的向量是否低于阈值
 * 若低于阈值，返回的结构体row为0
 * 否则即为可用的矩阵
 */
MATRIX processing_data(double *arr)
{
    int index = 0;
    double temp[SUM_DATA_LEN][COL];
    MATRIX result;
    while (index < TOTAL)
    {
        double sum = 0;
        for (int i = 0; i < SUM_DATA_LEN; i++)
        {
            for (int j = 0; i < COL; j++)
            {
                temp[i][j] = arr[index];
                sum += arr[index];
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
    if (DATA.row < TOTAL)
    {
        DATA.row++;
    }
    DATA.index++;
    if (DATA.index == TOTAL)
    {
        DATA.index = 0;
    }
    for (int i = 0; i < COL; i++)
    {
        DATA.arr[DATA.index][i] = (double)received_data[i];
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

SIMILARITY action_test(int res[][COL], int received_data[7])
{
    SIMILARITY similarity = {-1, -1};
    update_data(received_data);
    if (DATA.row <= TOTAL)
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
        int _res[COL];
        for(int j=0;j<COL;j++)
        {
            _res[j]=res[i][j];
        }
        sim[i] = cosine_similarity(_res, test.array, COL);
        if (sim[i] > sim[max])
        {
            max = i;
        }
    }
    similarity.action_id = max;
    similarity.max_similarity = sim[max];
    return similarity;
}

int main()
{
    double arr[2][2] = {{1, 2},
                        {3, 4}};
    MATRIX m = {(double *)arr, 2, 2};
    MATRIX result = mean(m);
    for (int i = 0; i < 2; i++)
    {
        printf("%f\n", result.array[i]);
    }
    return 0;
}
