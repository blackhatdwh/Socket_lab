#ifndef SERVER_H
#define SERVER_H
#include "common.h"
#endif


extern int errno;
char* server_ip = "127.0.0.1";      // define the server ip address
int sockfd;
int connected = 0;
pthread_t thread_id;

int status_code = 0;
int receiver = 0;
char msg_content[MAX_LENGTH];



void Disconnect(int recv_fd){
    printf("Server disconnected!\n");
    connected = 0;
    close(sockfd);
    pthread_cancel(thread_id);
}

void* SubProcess(void* args){
    pthread_detach(pthread_self());
    char recv_buffer[MAX_LENGTH];
    Reset(recv_buffer);
    while(1){
        int tmp_arr[] = {sockfd};
        ReceiveData(recv_buffer, sockfd, tmp_arr, 1);
        // printf("%s\n", recv_buffer);
        RawPrint(recv_buffer);

        sscanf(recv_buffer, "status_code: %d", &status_code);
        // send msg
        if(status_code == 100){
            char send_buffer[MAX_LENGTH] = "number: ";
            char receiver_text[10];
            sprintf(receiver_text, "%d", receiver);
            strcat(send_buffer, receiver_text);
            strcat(send_buffer, "\rmessage: ");
            strcat(send_buffer, msg_content);
            strcat(send_buffer, "\n");
            SendData(sockfd, send_buffer);
        }

        Reset(recv_buffer);
    }
}

void Connect(){
    char server_ip[20] = "127.0.0.1";
    int server_port = 4577;
    /*
    printf("Please specify server's IP and port.\n");
    printf("IP:");
    scanf("%s", server_ip);
    printf("\nport:");
    scanf("%d", &server_port);
    */

    struct sockaddr_in serverAddr;

    // create a socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket() failed! code:%d\n", errno);
        return;
    }

    // set up server's information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_port);
    serverAddr.sin_addr.s_addr = inet_addr(server_ip);
    bzero(&(serverAddr.sin_zero), 8);

    // send connection request to server
    printf("connecting!\n");
    if(connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        printf("connect() failed! code:%d\n", errno);
        close(sockfd);
        return;
    }

    printf("Connected!\n");
    connected = 1;
    int rc = pthread_create(&thread_id, NULL, SubProcess, NULL);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
}

int main(int argc, char *argv[]) {	
    while(1){
        printf("Choose a function:\n");
        printf("1.connect\n2.disconnect\n3.get time\n4.get name\n5.get list\n6.send msg\n7.exit\n");
        int choice = 0;
        scanf("%d", &choice);
        // connect
        if(choice == 1){
            Connect();
        }

        // disconnect
        if(choice == 2){
            Disconnect(sockfd);
        }


        // get time
        if(choice == 3){
            SendData(sockfd, function_code[0]);
        }

        // get name
        if(choice == 4){
            SendData(sockfd, function_code[1]);
        }

        // get list
        if(choice == 5){
            SendData(sockfd, function_code[2]);
        }

        // send msg
        if(choice == 6){
            printf("Please specify the receiver's number.\n");
            scanf("%d", &receiver);
            printf("Please specify the msg content.\n");
            scanf("%s", msg_content);
            SendData(sockfd, function_code[3]);
        }

        // exit
        if(choice == 7){
            exit(0);
        }
    }

    close(sockfd);//关闭套接字
    return 0;
}
