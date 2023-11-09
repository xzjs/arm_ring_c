#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tool.h"

int main()
{
    // FILE *file = fopen("test_primitive.csv", "r"); // 打开CSV文件，替换为你的文件名
    FILE *file = fopen("test_useable.csv", "r"); // 打开CSV文件，替换为你的文件名
    double res[6][8] = {
        {1.0325000000000002, 46289.76125, 1188.630625, 12484.528125,
         50423.027500000004, 3325.4518749999993, 0.24750000000000005,
         58097.395625000005},
        {5569.365624999999, 948.434375, 1550.0731250000001, 9133.459375,
         12903.298750000002, 4659.593125000001, 0.21624999999999997,
         10003.977499999997},
        {0.89875, 1843.1312499999997, 11003.106249999997, 2774.50875,
         260120.36187499994, 4801.465000000001, 0.230625, 48354.55687500001},
        {1.1493749999999998, 596.5712500000002, 1292.7049999999997, 654.640625,
         4398.098749999999, 736.721875, 0.25875, 3215.1418750000007},
        {1.206875, 87287.31874999999, 673.3356249999999, 84161.22, 1985.610625,
         6505.876250000001, 0.2174999999999999, 3480.5143749999997},
        {1.1275000000000004, 14056.70625, 4854.341249999999, 47172.59625, 22329.4575,
         0.0, 0.168125, 16365.93875}};

    if (file == NULL)
    {
        perror("无法打开文件");
        return 1;
    }

    char line[1024];              // 用于存储每一行的字符串
    char *token;                  // 用于分隔字符串的标记
    const char delimiter[] = ","; // CSV文件的分隔符
    int vector[8];
    int now = 0; // 当前读到的行数

    while (fgets(line, sizeof(line), file) != NULL)
    {
        printf("file %d \n", now++);
        // 从文件中读取一行数据
        // 使用strtok函数按逗号分隔字符串
        token = strtok(line, delimiter);
        int i = 0;
        while (token != NULL)
        {
            vector[i++] = atoi(token);
            token = strtok(NULL, delimiter);
        }
        SIMILARITY sim = action_test(res, vector);
        if (sim.action_id >= 0)
        {
            printf("%d,%f", sim.action_id, sim.max_similarity);
            DATA.index = 0;
            DATA.row = 0;
        }
    }

    fclose(file); // 关闭文件

    return 0;
}