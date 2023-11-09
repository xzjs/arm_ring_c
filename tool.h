#ifndef TOOL
#define TOOL

#define SUM_DATA_LEN 50
#define COL 8
#define ROW 1000
#define SUM_DATA_AVG_THRESHOLD 5000
#define WINDOW_SIZE_TRAIN 20
#define RATE_TRAIN 0.8
#define TOTAL 8000

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

SIMILARITY action_test(double res[][COL], int received_data[COL]);

#endif