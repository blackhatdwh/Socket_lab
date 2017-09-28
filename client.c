#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>

#define SERVER_PORT	4577 //侦听端口
#define MAX_LENGTH 1024 // max length of a packet

extern int errno;
char* server_ip = "127.0.0.1";      // define the server ip address
// used to indicate which function the client is requesting for
// 0: error, 1: get time, 2: get name, 3: get list, 4: send msg
char* function_code[4] = { "1\n", "2\n", "3\n", "4\n" };
char recv_buffer[MAX_LENGTH];       // a buffer used to store the income data
void* ptr;
int sockfd;

void Refresh(){
    for(int i = 0; i < MAX_LENGTH; i++){
        recv_buffer[i] = 0;
    }
    ptr = recv_buffer;
}

void ReceiveData(){
    int count = -1;
    int ret;
    // write the received data into recv_buffer
	ptr = recv_buffer;
    ret = recv(sockfd, ptr, MAX_LENGTH, 0);
    ptr = (char *)ptr + ret;
    count += ret;
    if(count == -1){
        return;
    }
    // a complete message always ends with a character '\n'
	while(recv_buffer[count] != '\n') {
		// receive data
		ret = recv(sockfd, ptr, MAX_LENGTH, 0);
        count += ret;
        // if some error happens, exit
		if(ret <= 0) {
			printf("recv() failed!\n");
			close(sockfd);// close the socket
			close(sockfd);
			exit(-1);
		}
		ptr = (char *)ptr + ret;
	}
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
    ReceiveData();
    printf("%s\n", recv_buffer);
    Refresh();
    }

	close(sockfd);//关闭套接字
	return 0;
}
