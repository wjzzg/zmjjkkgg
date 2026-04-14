/*===============================================
*   文件名称：a.c
*   创 建 者：     
*   创建日期：2026年04月14日
*   描    述：
================================================*/
#include <stdio.h>
#include <string.h>
// 必须添加的文件IO头文件
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    // 定义文件描述符
    int fd;
    // 定义缓冲区，用于存储读取的数据
    char buf[1024] = {0};

    // 打开文件：读写模式，不存在则创建，权限0664
    fd = open("1.txt", O_RDWR | O_CREAT, 0664);

    // 判断文件是否打开成功
    if (-1 == fd) {
        // 打开失败，打印错误信息
        perror("open fail");
        // 异常退出程序
        return -1;
    }

    // 从文件中读取数据到缓冲区
    ssize_t ret = read(fd, buf, sizeof(buf)-1);
    if (-1 == ret) {
        perror("read fail");
        close(fd);
        return -1;
    }

    // 打印读取到的内容
    printf("读取到的文件内容：\n%s\n", buf);

    // 关闭文件
    close(fd);

    return 0;
}