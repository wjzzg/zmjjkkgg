/*===============================================
*   文件名称：a.c
*   创 建 者：     
*   创建日期：2026年04月14日
*   描    述：
================================================*/
#include <stdio.h>

int main(int argc, char *argv[])
{ 
   int fd;
   fd = open("1.txt",O_RDWR | O_CREAT,0664);

   if(-1 == fd){
    perror("read fail\n");
   }

   printf("%s\n",buf);
   close(fd);

    return 0;
}
