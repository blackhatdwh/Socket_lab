#ifndef SERVER_H
#define SERVER_H
#include "common.h"
#include <time.h>
#endif

extern int errno;
int sockfd;

// used to store connection record
struct connection_log {
    struct connection_log* next;
    int comfd;
    char ip_addr[20];
    int port;
    int num;
    pthread_t thread_id;
};
struct connection_log* llhead;

// when user press Ctrl+C
// cancel all sub thread, free the connection log linked lisr, then exit
void signal_callback_handler(int signum){
    printf("killed by ctrl+C!\n");
    struct connection_log* llptr = llhead->next;
    while(llptr != NULL){
        pthread_cancel(llptr->thread_id);
        struct connection_log* tmp_ptr = llptr;
        llptr = llptr->next;
        free(tmp_ptr);
    }
    free(llhead);
    exit(0);
}

// insert a new record into the connection record list
int InsertRecord(int comfd, char* ip_addr, int port, pthread_t thread_id){
    struct connection_log* llptr = llhead;
    // traverse to the tail of the linked list
    while(llptr->next != NULL){
        llptr = llptr->next;
    }
    int num = llptr->num;     // get the index of the last element
    // create a new node at the tail of the linked list
    struct connection_log* new_node = (struct connection_log*)malloc(sizeof(struct connection_log));
    new_node->next = NULL;
    new_node->comfd = comfd;
    new_node->port = port;
    new_node->num = num + 1;
    new_node->thread_id = thread_id;
    strcpy(new_node->ip_addr, ip_addr);
    llptr->next = new_node;
}

void BadRequest(int send_fd){
    char* send_buffer = "status_code: 400\n";
    SendData(send_fd, send_buffer);
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
    SendData(send_fd, send_buffer);
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
    SendData(send_fd, send_buffer);
}

void ShowList(int send_fd){
    char send_buffer[MAX_LENGTH];
    sprintf(send_buffer, "status_code: 200\rcontent: ");
    struct connection_log* llptr = llhead->next;
    while(llptr != NULL){
        char list_element[80] = "Number: ";
        char num_text[5];
        sprintf(num_text, "%d", llptr->num);
        strcat(list_element, num_text);
        strcat(list_element, "\tIP: ");
        strcat(list_element, llptr->ip_addr);
        char port_text[5];
        sprintf(port_text, "%d", llptr->port);
        strcat(list_element, "\tPort: ");
        strcat(list_element, port_text);
        strcat(list_element, "\n");
        strcat(send_buffer, list_element);
        llptr = llptr->next;
    }
    int length = strlen(send_buffer);
    send_buffer[length] = '\n';
    send_buffer[length+1] = 0;
    SendData(send_fd, send_buffer);
}

// after receive client's send msg request, return a '100' to inform it to continue
// so the client will send more data
void TransferMsg_1(int send_fd){
    char send_buffer[MAX_LENGTH];
    sprintf(send_buffer, "status_code: 100\n");
    SendData(send_fd, send_buffer);
}

// after receiving extended data from the client, analyse it to get the destination and msg content
// and try to perform the transfering work
void TransferMsg_2(char* recv_buffer){
    int dest_num = 0;
    int dest_comfd = -1;
    char msg_content[MAX_LENGTH];
    sscanf(recv_buffer, "number: %d\rmessage: %s\n", &dest_num, msg_content);
    struct connection_log* llptr = llhead->next;
    while(llptr != NULL){
        if(llptr->num == dest_num){
            dest_comfd = llptr->comfd;
            break;
        }
        llptr = llptr->next;
    }
    // if the given number is not in the connecting list, response "400\n"
    if(dest_comfd == -1){
        char send_buffer[MAX_LENGTH] = "status_code: 400\n";
        SendData(dest_comfd, send_buffer);
        return;
    }
    char send_buffer[MAX_LENGTH] = "content: ";
    strcat(send_buffer, msg_content);
    strcat(send_buffer, "\n");
    SendData(dest_comfd, send_buffer);
}

// waiting for destination client to response
// if the dest client accepted the msg, it will response "200\n", then the server forward it to the original client
// if the dest client reject the msg, it will response "400\n", then the server will send "401\n" to the original client
void TransferMsg_3(int comfd, char* recv_buffer){
    int status_code = 0;
    sscanf(recv_buffer, "status_code: %d\n", &status_code);
    if(status_code == 200){
        char send_buffer[MAX_LENGTH] = "status_code: 200\n";
        SendData(comfd, send_buffer);
        return;
    }
    if(status_code == 400){
        char send_buffer[MAX_LENGTH] = "status_code: 401\n";
        SendData(comfd, send_buffer);
        return;
    }
}

// when a client disconnect, delete it from the connecting list
// also, reallocate the index of each client
void Disconnect(int recv_fd){
    // variable count is used to rearrange the index
    int count = 1;
    struct connection_log* pre_ptr = llhead;
    struct connection_log* llptr = llhead->next;
    // search for the node which represent the disconnected client
    while(llptr != NULL){
        llptr->num = count++;
        if(llptr->comfd == recv_fd){
            // find it and delete it
            pre_ptr->next = llptr->next;
            struct connection_log* tmp_ptr = llptr;
            llptr = llptr->next;
            free(tmp_ptr);
            count--;
            continue;
        }
        // traverse the linked list
        pre_ptr = pre_ptr->next;
        llptr = llptr->next;
    }
    pthread_exit(NULL);
}

// which function does the income data indicate?
int ParseFunction(char* recv_buffer){
    for(int i = 0; i < 4; i++){
        if(strcmp(function_code[i], recv_buffer) == 0){
            return i + 1;
        }
    }
    return 0;
}

void* SubProcess(void* raw_comfd){
    int comfd = *(int*)raw_comfd;
    char recv_buffer[MAX_LENGTH];   // a buffer used to store the income data
    Reset(recv_buffer);
    while(1){
        int tmp_arr[] = {comfd, sockfd};
        ReceiveData(recv_buffer, comfd, tmp_arr, 2);
        int function_code = ParseFunction(recv_buffer);
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
                ShowList(comfd);
                break;
            // send msg
            case 4:
                TransferMsg_1(comfd);
                Reset(recv_buffer);
                ReceiveData(recv_buffer, comfd, tmp_arr, 2);
                TransferMsg_2(recv_buffer);
                Reset(recv_buffer);
                ReceiveData(recv_buffer, comfd, tmp_arr, 2);
                TransferMsg_3(comfd, recv_buffer);
                break;
        }
        Reset(recv_buffer);
    }
}

int main() {	
    signal(SIGINT, signal_callback_handler);
    struct sockaddr_in serverAddr, clientAddr;
    int iClientSize;
    // connection records are stored in a linked list
    // initialize the linked list
    llhead = (struct connection_log*)malloc(sizeof(struct connection_log));
    llhead->next = NULL;
    llhead->num = 0;

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

    // listen
    if(listen(sockfd, 5) == -1) {
        printf("listen() failed! code:%d\n", errno);
        close(sockfd);
        return -1;
    }

    printf("Waiting for client connecting!\n");
    printf("tips : Ctrl+c to quit!\n");

    // accept connection request from client
    iClientSize = sizeof(struct sockaddr_in);
    while(1){
        int comfd;
        if((comfd = accept(sockfd, (struct sockaddr *)&clientAddr,(socklen_t *) &iClientSize)) == -1) {
            printf("accept() failed! code:%d\n", errno);
            close(sockfd);
            return -1;
        }
        printf("Accepted client: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        pthread_t thread_id;
        int rc = pthread_create(&thread_id, NULL, SubProcess, (void*)&comfd);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
        InsertRecord(comfd, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), thread_id);
    }

    // PROTOCOL RELATED WORK STARTS FROM HERE

    //close(sockfd);
    //close(comfd);
    return 0;
}
