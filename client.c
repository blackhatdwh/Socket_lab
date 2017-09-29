#ifndef SERVER_H
#define SERVER_H
#include "common.h"
#endif


extern int errno;
char* server_ip = "127.0.0.1";      // define the server ip address
int sockfd;


char recv_buffer[MAX_LENGTH];

void Disconnect(int recv_fd){
    printf("Server disconnected!\n");
}

int main(int argc, char *argv[]) {	
    struct sockaddr_in serverAddr;

    // 创建一个socket：
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket() failed! code:%d\n", errno);
        return -1;
    }

    // 设置服务器的地址信息：
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(server_ip);
    bzero(&(serverAddr.sin_zero), 8);

    //客户端发出连接请求：
    printf("connecting!\n");
    if(connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        printf("connect() failed! code:%d\n", errno);
        close(sockfd);
        return -1;
    }

    printf("Connected!\n");
    while(1){
        printf("Choose a function:\n");
        printf("1.get time\t2.get name\n3.get list\t4.send msg\n");
        int choice = 0;
        scanf("%d", &choice);

        // get time
        if(choice == 1){
            if(send(sockfd, function_code[0], sizeof(function_code[0]), 0) == -1) {
                printf("send() failed!\n");
                close(sockfd);
                return -1;
            }
        }

        // get name
        if(choice == 2){
            if(send(sockfd, function_code[1], sizeof(function_code[1]), 0) == -1) {
                printf("send() failed!\n");
                close(sockfd);
                return -1;
            }
        }

        // get list
        if(choice == 3){
            if(send(sockfd, function_code[2], sizeof(function_code[2]), 0) == -1) {
                printf("send() failed!\n");
                close(sockfd);
                return -1;
            }
        }

        // send msg
        if(choice == 4){
            if(send(sockfd, function_code[3], sizeof(function_code[3]), 0) == -1) {
                printf("send() failed!\n");
                close(sockfd);
                return -1;
            }
        }
        Reset(recv_buffer);
        int tmp_arr[] = {sockfd};
        ReceiveData(recv_buffer, sockfd, tmp_arr, 1);
        printf("%s\n", recv_buffer);
        Reset(recv_buffer);
    }

    close(sockfd);//关闭套接字
    return 0;
}
