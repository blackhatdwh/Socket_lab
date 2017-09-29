#ifndef SERVER_H
#define SERVER_H
#include "common.h"
#include <time.h>
#endif

extern int errno;
int sockfd;

void BadRequest(int send_fd){
    char* send_buffer = "status_code: 400\n";
    if(send(send_fd, send_buffer, strlen(send_buffer), 0) == -1) {
        printf("send() failed!\n");
        close(send_fd);
        exit(-1);
    }
}

void ShowTime(int send_fd){
    char send_buffer[MAX_LENGTH];
    sprintf(send_buffer, "status_code: 200\rcontent: ");
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime ( &rawtime );
    char time_text[100];
    sprintf (time_text, "Current local time and date: %s\n", asctime (timeinfo));
    strcat(send_buffer, time_text);
    if(send(send_fd, send_buffer, strlen(send_buffer), 0) == -1) {
        printf("send() failed!\n");
        close(send_fd);
        exit(-1);
    }
}

void ShowName(int send_fd){
    char send_buffer[MAX_LENGTH];
    sprintf(send_buffer, "status_code: 200\rcontent: ");
    char host_name_text[100];
    gethostname(host_name_text, MAX_LENGTH);
    strcat(send_buffer, host_name_text);
    int length = strlen(send_buffer);
    send_buffer[length] = '\n';
    send_buffer[length+1] = 0;
    if(send(send_fd, send_buffer, strlen(send_buffer), 0) == -1) {
        printf("send() failed!\n");
        close(send_fd);
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

    // create a newsocket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket() failed! code:%d\n", errno);
        return -1;
    }

    // bind
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(serverAddr.sin_zero), 8);
    if(bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        printf("bind() failed! code:%d\n", errno);
        close(sockfd);
        return -1;
    }

    // listern
    if(listen(sockfd, 5) == -1) {
        printf("listen() failed! code:%d\n", errno);
        close(sockfd);
        return -1;
    }

    printf("Waiting for client connecting!\n");
    printf("tips : Ctrl+c to quit!\n");

    // accept connection request from client
    int comfd;
    char recv_buffer[MAX_LENGTH];   // a buffer used to store the income data
    iClientSize = sizeof(struct sockaddr_in);
    if((comfd = accept(sockfd, (struct sockaddr *)&clientAddr,(socklen_t *) &iClientSize)) == -1) {
        printf("accept() failed! code:%d\n", errno);
        close(sockfd);
        return -1;
    }

    printf("Accepted client: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    // PROTOCOL RELATED WORK STARTS FROM HERE

    while(1){

        int tmp_arr[] = {comfd, sockfd};
        ReceiveData(comfd, tmp_arr, 2);
        int function_code = ParseFunction();
        switch(function_code){
            // wrong function code
            case 0:
                BadRequest(comfd);
                break;
            // get time
            case 1:
                ShowTime(comfd);
                break;
            // get name
            case 2:
                ShowName(comfd);
                break;
            // get list
            case 3:
                break;
            // send msg
            case 4:
                break;
        }
        Reset();

    }
    close(sockfd);
    close(comfd);
    return 0;
}
