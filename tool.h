#ifndef TOOL
#define TOOL

#define SUM_DATA_LEN 50
#define COL 8
#define SUM_DATA_AVG_THRESHOLD 5000
#define WINDOW_SIZE_TRAIN 20
#define RATE_TRAIN 0.8

typedef struct similarity
{
    int action_id;            // 动作id
    double similarity[6]; // 相似度数组
} SIMILARITY;                 // 相似度

SIMILARITY action_test(double res[][COL], int received_data[COL]);

#endif