#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#define SERVER_PORT 4577
#define MAX_LENGTH 1024

extern int errno;
int sockfd, comfd;
char recv_buffer[MAX_LENGTH];       // a buffer used to store the income data
// used to indicate which function the client is requesting for
// 0: error, 1: get time, 2: get name, 3: get list, 4: send msg
char* function_code[4] = { "1\n", "2\n", "3\n", "4\n" };
void *ptr;

// used to clear the receive buffer
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
    ret = recv(comfd, ptr, MAX_LENGTH, 0);
    ptr = (char *)ptr + ret;
    count += ret;
    if(count == -1){
        return;
    }
    // a complete message always ends with a character '\n'
	while(recv_buffer[count] != '\n') {
		// receive data
		ret = recv(comfd, ptr, MAX_LENGTH, 0);
        count += ret;
        // if some error happens, exit
		if(ret <= 0) {
			printf("recv() failed!\n");
			close(sockfd);// close the socket
			close(comfd);
			exit(-1);
		}
		ptr = (char *)ptr + ret;
	}
}

void BadRequest(){
	char* send_buffer = "status_code: 400\n";
    if(send(comfd, send_buffer, strlen(send_buffer), 0) == -1) {
        printf("send() failed!\n");
        close(comfd);
        exit(-1);
    }
}

void ShowTime(){
	char send_buffer[MAX_LENGTH];
    sprintf(send_buffer, "status_code: 200\rcontent: ");
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime ( &rawtime );
    char time_text[100];
	sprintf (time_text, "Current local time and date: %s\n", asctime (timeinfo));
    strcat(send_buffer, time_text);
    if(send(comfd, send_buffer, strlen(send_buffer), 0) == -1) {
        printf("send() failed!\n");
        close(comfd);
        exit(-1);
    }
}

void ShowName(){
    char send_buffer[MAX_LENGTH];
    sprintf(send_buffer, "status_code: 200\rcontent: ");
    char host_name_text[100];
    gethostname(host_name_text, MAX_LENGTH);
    strcat(send_buffer, host_name_text);
    int length = strlen(send_buffer);
    send_buffer[length] = '\n';
    send_buffer[length+1] = 0;
    if(send(comfd, send_buffer, strlen(send_buffer), 0) == -1) {
        printf("send() failed!\n");
        close(comfd);
        exit(-1);
    }
}


// which function does the income data indicate?
int ParseFunction(){
    for(int i = 0; i < 4; i++){
        if(strcmp(function_code[i], recv_buffer) == 0){
            return i + 1;
        }
    }
    return 0;
}

int main() {	
	struct sockaddr_in serverAddr, clientAddr;
	int iClientSize;

	// 创建一个socket：
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("socket() failed! code:%d\n", errno);
		return -1;
	}

	// 绑定：
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(serverAddr.sin_zero), 8);
	if(bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
		printf("bind() failed! code:%d\n", errno);
		close(sockfd);
		return -1;
	}

	// 侦听控制连接请求：
	if(listen(sockfd, 5) == -1) {
		printf("listen() failed! code:%d\n", errno);
		close(sockfd);
		return -1;
	}

	printf("Waiting for client connecting!\n");
	printf("tips : Ctrl+c to quit!\n");

	//接受客户端连接请求：
	iClientSize = sizeof(struct sockaddr_in);
	if((comfd = accept(sockfd, (struct sockaddr *)&clientAddr,(socklen_t *) &iClientSize)) == -1) {
		printf("accept() failed! code:%d\n", errno);
		close(sockfd);
		return -1;
	}

	printf("Accepted client: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    // PROTOCOL RELATED WORK STARTS FROM HERE

    while(1){
    
    ReceiveData();
    int function_code = ParseFunction();
    switch(function_code){
        // wrong function code
        case 0:
            BadRequest();
            break;
        // get time
        case 1:
            ShowTime();
            break;
        // get name
        case 2:
            ShowName();
            break;
        // get list
        case 3:
            break;
        // send msg
        case 4:
            break;
    }
    Refresh();

    }
    close(sockfd);//关闭套接字
    close(comfd);
    return 0;
}
