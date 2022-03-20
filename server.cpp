#include <stdio.h>
#include <netinet/in.h> //for souockaddr_in
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <ctype.h>
//for select
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include <strings.h> //for bzero
#include <string.h>
#define BUFF_SIZE 1024
#define backlog 7
#define ser_port 12021
#define CLI_NUM 30
#define CAN_MAXNUM 10
int client_fds[CLI_NUM];
int channel;
int main(int agrc, char **argv)
{
    int server_sockfd;
    int i;
    char input_message[BUFF_SIZE];
    char res_message[BUFF_SIZE];

    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET; //IPV4
    ser_addr.sin_port = htons(ser_port);
    ser_addr.sin_addr.s_addr = INADDR_ANY; //指定的是所有地址

    //creat socket
    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("creat failure");
        return -1;
    }
    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //启用端口复用
    //bind soucket
    if (bind(server_sockfd, (const struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
    {
        perror("bind failure");
        return -1;
    }

    //listen
    if (listen(server_sockfd, backlog) < 0)
    {
        perror("listen failure");
        return -1;
    }

    //fd_set
    fd_set ser_fdset;
    int max_fd = 1;
    struct timeval mytime;
    printf("wait for client connnect!\n");

    while (1)
    {
        mytime.tv_sec = 27;
        mytime.tv_usec = 0;

        FD_ZERO(&ser_fdset);

        //add standard input
        FD_SET(0, &ser_fdset);
        if (max_fd < 0)
        {
            max_fd = 0;
        }

        //add server
        FD_SET(server_sockfd, &ser_fdset);
        if (max_fd < server_sockfd)
        {
            max_fd = server_sockfd;
        }

        //add client
        for (i = 0; i < CLI_NUM; i++) //用数组定义多个客户端fd
        {
            if (client_fds[i] != 0)
            {
                FD_SET(client_fds[i], &ser_fdset);
                if (max_fd < client_fds[i])
                {
                    max_fd = client_fds[i];
                }
            }
        }

        //select多路复用
        int ret = select(max_fd + 1, &ser_fdset, NULL, NULL, &mytime);

        if (ret < 0)
        {
            perror("select failure\n");
            continue;
        }

        else if (ret == 0)
        {
            printf("time out!\n");
            continue;
        }

        else
        {
            if (FD_ISSET(0, &ser_fdset)) //标准输入是否存在于ser_fdset集合中（也就是说，检测到输入时，做如下事情）
            {
                printf("send message to:\n");
                bzero(input_message, BUFF_SIZE);
                fgets(input_message, BUFF_SIZE, stdin);

                for (i = 0; i < CLI_NUM; i++)
                {
                    if (client_fds[i] != 0)
                    {
                        printf("client_fds[%d]=%d\n", i, client_fds[i]);
                        send(client_fds[i], input_message, BUFF_SIZE, 0);
                    }
                }
            }

            if (FD_ISSET(server_sockfd, &ser_fdset))
            {
                struct sockaddr_in client_address;
                socklen_t address_len = sizeof(client_address);
                int client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &address_len);
                if (client_sockfd > 0)
                {
                    int flags = -1;//标记客户端
                    for (i = 0; i < CLI_NUM; i++)//CLI_NUM为最大客户端数量
                    {
                        if (client_fds[i] == 0)
                        {
                            client_fds[i] = client_sockfd;
                            flags = i;
                            break;
                        }
                    }
                    if (flags >= 0) //用户验证成功
                    {
                        printf("new user client[%d] add sucessfully!\n", flags);
                        char id[20];
                        char password[20];
                        char rpassword[20];
                        bzero(input_message, BUFF_SIZE);
                        sprintf(input_message, "your client number is %d,Register or log in?(R/L)\n", flags);
                        send(client_sockfd, input_message, BUFF_SIZE, 0);
                        bzero(res_message, BUFF_SIZE);
                        read(client_fds[i], res_message, BUFF_SIZE);
                        if (!strcmp(res_message, "R")) //register
                        {
                            FILE *fp = fopen("id.txt", "a+");
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "your client number is %d,Please input your ID:\n", flags);
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            bzero(res_message, BUFF_SIZE);
                            bzero(id, 20);
                            read(client_sockfd, id, 20);
                            fprintf(fp, "%s\n", id);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "Your ID is %s\nPlease input your Password:\n", id);
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            bzero(password, 20);
                            read(client_fds[i], password, 20);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "Please repeat your Password:\n");
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            bzero(rpassword, 20);
                            read(client_fds[i], rpassword, 20);
                            while (strcmp(rpassword, password))
                            {
                                bzero(input_message, BUFF_SIZE);
                                sprintf(input_message, "The password and confirmation password are different.\n Please double-check these fields and try again.\n");
                                send(client_sockfd, input_message, BUFF_SIZE, 0);
                                bzero(input_message, BUFF_SIZE);
                                sprintf(input_message, "Your ID is %s\nPlease input your Password:\n", id);
                                send(client_sockfd, input_message, BUFF_SIZE, 0);
                                bzero(password, 20);
                                read(client_fds[i], password, 20);
                                bzero(input_message, BUFF_SIZE);
                                sprintf(input_message, "Please repeat your Password:\n");
                                send(client_sockfd, input_message, BUFF_SIZE, 0);
                                bzero(rpassword, 20);
                                read(client_fds[i], rpassword, 20);
                            }
                            fprintf(fp, "%s\n", password);
                            fclose(fp);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "Register success!\n");
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                        }
                        int tcount = 0;
                        while (tcount < 5&&tcount>=0)
                        {
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "your client number is %d,Please input your ID:\n", flags);
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            bzero(res_message, BUFF_SIZE);
                            bzero(id, 20);
                            read(client_sockfd, id, 20);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "Your ID is %s\nPlease input your Password:\n", id);
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            bzero(password, 20);
                            read(client_fds[i], password, 20);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "your Password is %s\n", password);
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            FILE *login = fopen("id.txt", "r");
                            char idset[20];
                            char passwordset[20];
                            while (EOF != fscanf(login, "%s", idset))
                            {
                                if (EOF != fscanf(login, "%s", passwordset))
                                {
                                    if (!strcmp(id, idset) && !strcmp(password, passwordset))
                                    {
                                        bzero(input_message, BUFF_SIZE);
                                        sprintf(input_message, "id correct!\npassword correct!\nWelcome!\n");
                                        std::cout << input_message;
                                        send(client_sockfd, input_message, BUFF_SIZE, 0);
                                        tcount = -2;
                                        break;
                                    }
                                }
                            }
                            tcount++;
                            if (tcount >=0&&tcount<5)
                            {
                                bzero(input_message, BUFF_SIZE);
                                sprintf(input_message, "Your ID or password is not correct!Please try again!You still have %d chance to try！\n", 5 - tcount);
                                std::cout << input_message;
                                send(client_sockfd, input_message, BUFF_SIZE, 0);
                            }
                        }
                        if (tcount == 5)
                        {
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "try too much！Press any key to exit!\n");
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "quit");
                            send(client_sockfd, input_message, BUFF_SIZE, 0);
                            FD_CLR(client_fds[i], &ser_fdset);
                            client_fds[i] = 0;
                            continue;
                        }
                    }
                    else //flags=-1
                    {
                        char full_message[] = "the client is full!\n";
                        bzero(input_message, BUFF_SIZE);
                        strncpy(input_message, full_message, 100);
                        send(client_sockfd, input_message, BUFF_SIZE, 0);
                    }
                }
            }
        }

        //deal with the message

        for (i = 0; i < CLI_NUM; i++)
        {
            if (client_fds[i] != 0)
            {
                if (FD_ISSET(client_fds[i], &ser_fdset))
                {
                    bzero(res_message, BUFF_SIZE);
                    int byte_num = read(client_fds[i], res_message, BUFF_SIZE);
                    if (byte_num > 0)
                    {   if(!strcmp(res_message, "quit")){
                            printf("clien[%d] exit!\n", i);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "quit");
                            send(client_fds[i], input_message, BUFF_SIZE, 0);
                            FD_CLR(client_fds[i], &ser_fdset);
                            client_fds[i] = 0;
                            continue;
                        }
                        if (!strcmp(res_message, "download"))
                        { //发送文件
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "filesend");
                            send(client_fds[i], input_message, BUFF_SIZE, 0);
                            std::cout << "client[" << i << "]ask for download!" << std::endl;
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "Please input download file name:\n");
                            send(client_fds[i], input_message, BUFF_SIZE, 0);
                            char filename[BUFF_SIZE / 2];
                            bzero(filename, BUFF_SIZE / 2);
                            read(client_fds[i], filename, BUFF_SIZE / 2);
                            send(client_fds[i], filename, BUFF_SIZE / 2, 0);
                            std::cout << filename << std::endl;
                            char buf[1024] = {0};
                            FILE *fp = fopen(filename, "rb");

                            if (fp == NULL)
                            {
                                printf("Cannot open file, press try again\n");
                                continue;
                            }
                            fseek(fp, 0, SEEK_END);
                            int size = ftell(fp);
                            rewind(fp);
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "%d", size);
                            send(client_fds[i], input_message, sizeof(input_message), 0);
                            printf("size=%d\n", size);
                            printf("The transmission begins!\n");
                            int length;
                            char stflag = 0; //准备作为传送中断标志（未启用）
                            int flength = size;
                            while ((length = fread(buf, 1, BUFF_SIZE, fp)) > 0)
                            { //循环发送文件
                                send(client_fds[i], buf, length, 0);
                                std::cout << "Progress:" << 100.00 - flength * 100.00 / size * 1.00 << std::endl;
                                flength -= length;
                            }
                            fclose(fp);
                            printf("the file transmitted successfully!\n");
                        }
                        else if (!strcmp(res_message, "upload"))
                        { //接收文件
                            char filename[BUFF_SIZE / 2];
                            bzero(input_message, BUFF_SIZE);
                            sprintf(input_message, "filereceive");
                            send(client_fds[i], input_message, BUFF_SIZE, 0);
                            std::cout << "client[" << i << "]ask for upload!" << std::endl;
                            bzero(filename, BUFF_SIZE / 2);
                            read(client_fds[i], filename, BUFF_SIZE / 2);
                            send(client_fds[i], filename, BUFF_SIZE / 2, 0);
                            int size;
                            memset(input_message, 0, sizeof(input_message));
                            recv(client_fds[i], input_message, sizeof(input_message), 0);
                            size = atoi(input_message);
                            printf("size=%d\n", size);
                            int flength = size;
                            FILE *fp = fopen(filename, "wb"); //以二进制方式打开（创建）文件
                            if (fp == NULL)
                            {
                                printf("Cannot open file\n");
                                continue;
                            }
                            char buf[BUFF_SIZE] = {0}; //文件缓冲区
                            int length;
                            while (flength > 0 && (length = recv(client_fds[i], buf, sizeof(buf), 0)) > 0)
                            {
                                fwrite(buf, length, 1, fp);
                                memset(buf, 0, BUFF_SIZE);
                                std::cout << "Progress:" << 100.00 - flength * 100.00 / size * 1.00 << std::endl;
                                flength -= length;
                            }
                            fclose(fp);
                            printf("File transfer success!\n");
                        }
                        printf("message from client[%d]:%s\n", i, res_message);
                        int rec = 0;
                        int k = 0;
                        int flag = 0;
                        for (k; k < strlen(res_message); k++)
                        {
                            if (isdigit(res_message[k]))
                            {
                                flag = 1;
                                rec = rec * 10 + res_message[k] - '0';
                            }
                            else
                            {
                                k++;
                                break;
                            }
                        }
                        if (flag == 1)
                        { //有客户端指向
                            char tmp_message[1024];
                            char origin_message[1024];
                            sprintf(origin_message, "From client %d:\n", i);
                            bzero(tmp_message, BUFF_SIZE);
                            strncpy(tmp_message, res_message + k, strlen(res_message) - k);
                            printf("client_fds[%d] to %d\n", i, rec);
                            send(client_fds[rec], origin_message, BUFF_SIZE, 0); //转发头信息
                            send(client_fds[rec], tmp_message, BUFF_SIZE, 0);    //转发文本
                        }
                    }
                    else if (byte_num < 0)
                    {
                        printf("rescessed error!\n");
                    }

                    //某个客户端退出
                    else //cancel fdset and set fd=0
                    {
                        printf("clien[%d] exit!\n", i);
                        FD_CLR(client_fds[i], &ser_fdset);
                        client_fds[i] = 0;
                        continue;
                    }
                }
            }
        }
    }
    close(server_sockfd);
    return 0;
}
