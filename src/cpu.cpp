#include "cpu.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

void switch_cpu_set(char core_type) {
    pid_t pid = getpid();
    FILE* file = NULL;
    char proc_path_ai[] = "/proc/set_ai_thread";
    char proc_path_regular[] = "/proc/set_regular_thread";

    // 以写入模式打开目标文件
    if ('a' == core_type) {
        file = fopen(proc_path_ai, "w");
    }
    else if ('x' == core_type) {
        file = fopen(proc_path_regular, "w");
    }
    else {
        return;
    }
    if (file == NULL) {
        // 打开文件失败，通常意味着没有权限或文件不存在
        perror("错误: 无法打开 /proc/set_ai_thread");
        return;
    }

    // 将PID格式化并写入文件
    // 使用 %d 格式符处理常见的整数型PID
    if (fprintf(file, "%d", pid) < 0) {
        // 写入操作失败
        perror("错误: 写入 PID 到 /proc/set_ai_thread 失败");
        fclose(file);
        return;
    }

    // 关闭文件句柄
    if (fclose(file) != 0) {
        // 关闭文件失败
        perror("错误: 关闭 /proc/set_ai_thread 文件失败");
        return; // 虽然写入已成功，但关闭失败也应视为一种错误
    }
    sched_yield();
    // 操作成功完成
    return;
}

void query_cpu() {
    int current_cpu = sched_getcpu();
    printf("[QUERY_CPU] current CPU id is: %d\n", current_cpu);
}
