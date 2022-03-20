//编译时需外挂lpthread库，例如：g++ -o client client1.cpp -lpthread
//执行 ./client 127.0.0.1 8080
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <error.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <strings.h> //for bzero
#include <string.h>
#include <iostream>
#define BUFF_SIZE 1024
int flag=0;
void download(int st, char *s)
{
    char filename[BUFF_SIZE / 2] = {0}; //文件名
    int size;
    recv(st, s, BUFF_SIZE, 0);
    printf("%s\n", s);
    recv(st, filename, sizeof(filename), 0);
    memset(s, 0, BUFF_SIZE);
    recv(st, s, BUFF_SIZE, 0);
    size = atoi(s);
    printf("length=%d", size);
    FILE *fp = fopen(filename, "wb"); //以二进制方式打开（创建）文件
    if (fp == NULL)
    {
        printf("Cannot open file\n");
        return;
    }
    printf("open success!\n");
    char buf[BUFF_SIZE] = {0};
    int flength = size; //文件缓冲区
    int length;
    while (flength > 0 && (length = recv(st, buf, sizeof(buf), 0)) > 0)
    {
        fwrite(buf, length, 1, fp);
        std::cout << "Progress:" << 100.00 - flength * 100.00 / size << std::endl;
        memset(buf, 0, BUFF_SIZE);
        flength -= length;
    }
    fclose(fp);
    printf("File transfer success!\n");
    return;
}
void upload(int st, char *s)
{
    char filename[BUFF_SIZE / 2] = {0}; //文件地址
    char buf[1024] = {0};
    printf("please input the position of your file:\n");
    recv(st, filename, sizeof(filename), 0);
    printf("filename sucess!\n");
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Cannot open file, press try again\n");
        return;
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    bzero(s, BUFF_SIZE);
    sprintf(s, "%d", size);
    send(st, s, sizeof(s), 0);
    printf("The transmission begins, please wait, if you want to stop, please input s!\n");
    int length;
    int flength = size;
    char stflag = 0; //准备作为传送中断标志（未启用）
    while ((length = fread(buf, 1, BUFF_SIZE, fp)) > 0)
    { //循环发送文件
        send(st, buf, length, 0);
        std::cout << "Progress:" << 100.00 - flength * 100.00 / size * 1.00 << std::endl;
        flength -= length;
    }
    fclose(fp);
    printf("the file transmitted successfully!\n");
    return;
}
void *recvsocket(void *arg) //接受来着客户端数据的线程
{
    int st = *(int *)arg;
    char s[BUFF_SIZE];
    while (flag!=1)
    {
        memset(s, 0, sizeof(s));
        int rc = recv(st, s, sizeof(s), 0);
        if (rc <= 0) //代表socket被关闭（0）或者出错（-1）
        {
            break;
        }
        if (!strcmp(s, "filesend"))
        {
            download(st, s);
        }
        else if (!strcmp(s, "filereceive"))
        {
            upload(st, s);
        }
        else if(!strcmp(s,"quit")){
            flag=1;
            close(st);
            return EXIT_SUCCESS;
        }
        else{
            printf("%s\n", s);
        }
    }
    return NULL;
}
void *sendsocket(void *arg) //向服务端socket发送数据的线程
{
    int st = *(int *)arg;
    char s[1024];
    while (flag!=1)
    {
        memset(s, 0, sizeof(s));
        scanf("%s", s);
        if(!strcmp(s,"quit")){
            printf("Are you sure to leave?(Y/N)");
            scanf("%s", s);
            if(!strcmp(s,"Y")){
                sprintf(s,"quit");
                send(st,s,sizeof(s),0);
                flag=1;
                close(st);
                return EXIT_SUCCESS;
            }
            else{
                continue;
            }
        }
        int sc = send(st, s, strlen(s), 0);
    }
    return NULL;
}

int main(int arg, char *args[])
{
    if (arg < 3)
    {
        printf("arg<3\n");
        return -1;
    }
    int port = atoi(args[2]);
    //初始化socket
    int st = socket(AF_INET, SOCK_STREAM, 0);

    //定义一个IP地址结构并设置值
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));//分配内存并初始化
    addr.sin_family = AF_INET;//使用TCP/IP协议
    addr.sin_port = htons(port);//port
    addr.sin_addr.s_addr = inet_addr(args[1]);//ip

    //连接服务端
    if (connect(st, (struct sockaddr *)&addr, sizeof(addr)) < 0)//连接失败
    {
        printf("connect fail %s\n", strerror(errno));
        close(st);
        return EXIT_FAILURE;
    }

    //建立多线程传输
    pthread_t thrd1, thrd2; //定义一个线程
    pthread_create(&thrd1, NULL, recvsocket, &st);
    pthread_create(&thrd2, NULL, sendsocket, &st);
    pthread_join(thrd1, NULL);
    pthread_join(thrd2, NULL);
    close(st);
    return EXIT_SUCCESS;
}