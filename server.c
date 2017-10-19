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

int sub2for[2];     // pipe from subthreads to forwarder
int for2sub[2];     // pipe from forwarder to subthreads  
int destination = -1;       // indicates which subthread is the destination
int third_party_response = 0;      // response from the third party client in a msg sending process. 0 represents reject while 1 represents accept

pthread_mutex_t destination_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connection_log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t function_code_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t pipe_mutex = PTHREAD_MUTEX_INITIALIZER;     // lock to write into sub2for
pthread_cond_t pipe_cond = PTHREAD_COND_INITIALIZER;       // forwarder wait for subthread to write
pthread_mutex_t forwarder_mutex = PTHREAD_MUTEX_INITIALIZER;        // mutex for forwarder_cond
pthread_cond_t forwarder_cond = PTHREAD_COND_INITIALIZER;       // forwarder wait until the destination subthread is ready to receive data
pthread_mutex_t third_party_mutex = PTHREAD_MUTEX_INITIALIZER;      // mutex for third_party_cond
pthread_cond_t third_party_cond = PTHREAD_COND_INITIALIZER;      // wait for third party client to response

// when user press Ctrl+C
// cancel all sub thread, free the connection log linked lisr, then exit
void signal_callback_handler(int signum){
    printf("killed by ctrl+C!\n");

    pthread_mutex_lock(&connection_log_mutex);

    struct connection_log* llptr = llhead->next;
    while(llptr != NULL){
        pthread_cancel(llptr->thread_id);
        struct connection_log* tmp_ptr = llptr;
        llptr = llptr->next;
        free(tmp_ptr);
    }

    pthread_mutex_unlock(&connection_log_mutex);

    free(llhead);
    exit(0);
}

// insert a new record into the connection record list
int InsertRecord(int comfd, char* ip_addr, int port, pthread_t thread_id){
    pthread_mutex_lock(&connection_log_mutex);

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

    pthread_mutex_unlock(&connection_log_mutex);
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

    pthread_mutex_lock(&connection_log_mutex);

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
        strcat(list_element, "\r");
        strcat(send_buffer, list_element);
        llptr = llptr->next;
    }

    pthread_mutex_unlock(&connection_log_mutex);

    int length = strlen(send_buffer);
    send_buffer[length] = '\n';
    send_buffer[length+1] = 0;
    SendData(send_fd, send_buffer);
}

// a subprocess used to for forward msg among other subthreads
void* Forwarder(void* ptr){
    pipe(for2sub);
    pipe(sub2for);
    //close(sub2for[1]);      // close sub2for's write end
    char buffer[MAX_LENGTH];
    while(1){
        Reset(buffer);
        pthread_mutex_lock(&pipe_mutex);
        pthread_cond_wait(&pipe_cond, &pipe_mutex);
        pthread_mutex_unlock(&pipe_mutex);


        char* ptr = buffer;
        while(buffer[strlen(buffer) - 1] != '\n'){
            int length = read(sub2for[0], ptr, MAX_LENGTH);      // read from pipe
            if(length = -1){
                continue;
            }
            ptr += length;
        }

        pthread_mutex_lock(&destination_mutex);
        sscanf(buffer, "dest: %d\r", &destination);
        pthread_mutex_unlock(&destination_mutex);


        pthread_mutex_lock(&forwarder_mutex);
        pthread_cond_wait(&forwarder_cond, &forwarder_mutex);       // wait for dest subthread to response
        pthread_mutex_unlock(&forwarder_mutex);
        write(for2sub[1], buffer, strlen(buffer));      // write to the subthread
    }
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
// return the third party comfd
int TransferMsg_2(char* recv_buffer){
    int dest_num = 0;
    int dest_thread_id;
    int from_thread_id = pthread_self();
    int dest_comfd = -1;
    char msg_content[MAX_LENGTH];
    sscanf(recv_buffer, "number: %d\rmessage: %s\n", &dest_num, msg_content);

    pthread_mutex_lock(&connection_log_mutex);

    struct connection_log* llptr = llhead->next;
    while(llptr != NULL){
        if(llptr->num == dest_num){
            dest_comfd = llptr->comfd;
            dest_thread_id = llptr->thread_id;
            break;
        }
        llptr = llptr->next;
    }

    pthread_mutex_unlock(&connection_log_mutex);

    // if the given number is not in the connecting list, response "400\n"
    if(dest_comfd == -1){
        //char send_buffer[MAX_LENGTH] = "status_code: 400\n";
        //SendData(dest_comfd, send_buffer);
        //SendData(client_fd, send_buffer);
        return dest_comfd;
    }
    char send_buffer[MAX_LENGTH];
    sprintf(send_buffer, "dest: %d\rfrom: %d\rcontent: status_code: 300\rmessage: %s\n", dest_thread_id, from_thread_id, msg_content);
    pthread_mutex_lock(&pipe_mutex);
    pthread_cond_signal(&pipe_cond);        // wake up forwarder
    write(sub2for[1], send_buffer, strlen(send_buffer));
    pthread_mutex_unlock(&pipe_mutex);
    return dest_comfd;
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

    pthread_mutex_lock(&connection_log_mutex);

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

    pthread_mutex_unlock(&connection_log_mutex);

    pthread_exit(NULL);
}

// which function does the income data indicate?
int ParseFunction(char* recv_buffer){

    pthread_mutex_lock(&function_code_mutex);

    for(int i = 0; i < 4; i++){
        if(strcmp(function_code[i], recv_buffer) == 0){

            pthread_mutex_unlock(&function_code_mutex);

            return i + 1;
        }
    }

    pthread_mutex_unlock(&function_code_mutex);



    if(strcmp(recv_buffer, "status_code: 200\n") == 0){
        return 5;
    }
    if(strcmp(recv_buffer, "status_code: 400\n") == 0){
        return 6;
    }
    return 0;
}

struct tmp_struct{
    pthread_t thread_id;
    int comfd;
};

void* SubSubThread(void* args){
    struct tmp_struct* tmp = (struct tmp_struct*)args;
    pthread_t parent_thread_id = tmp->thread_id;
    int comfd = tmp->comfd;

    while(1){
        pthread_mutex_lock(&destination_mutex);
        if(destination == (int)parent_thread_id){
            pthread_mutex_unlock(&destination_mutex);
            char buffer[MAX_LENGTH];
            Reset(buffer);
            char* ptr = buffer;
            pthread_mutex_lock(&forwarder_mutex);
            pthread_cond_signal(&forwarder_cond);       // wake up the forwarder
            pthread_mutex_unlock(&forwarder_mutex);
    
            if(buffer[strlen(buffer) - 1] != '\n'){
                int length = read(for2sub[0], ptr, MAX_LENGTH);      // read from forwarder
                if(length == -1){
                    continue;
                }
                ptr += length;
            }

            int dest, from;
            char content[MAX_LENGTH];
            sscanf(buffer, "dest: %d\rfrom: %d\rcontent: %[^\n]s", &dest, &from, content);
            strcat(content, "\n");
            // printf("raw content:   ");
            // RawPrint(content);
            if(strcmp(content, "status_code: 200\n") == 0){
                SendData(comfd, content);
                pthread_mutex_lock(&destination_mutex);
                destination = -1;
                pthread_mutex_unlock(&destination_mutex);
                continue;
            }
            if(strcmp(content, "status_code: 400\n") == 0){
                char send_buffer[MAX_LENGTH] = "status_code: 401\n";
                SendData(comfd, send_buffer);
                pthread_mutex_lock(&destination_mutex);
                destination = -1;
                pthread_mutex_unlock(&destination_mutex);
                continue;
            }
            SendData(comfd, content);

            
            pthread_mutex_lock(&third_party_mutex);
            pthread_cond_wait(&third_party_cond, &third_party_mutex);       // wait for third party client to response
            pthread_mutex_unlock(&third_party_mutex);

            char internal_send_buffer[MAX_LENGTH];
            // if msg accepted
            if(third_party_response == 1){
                sprintf(internal_send_buffer, "dest: %d\rfrom: %d\rcontent: %s", from, dest, "status_code: 200\n");
            }
            else{
                sprintf(internal_send_buffer, "dest: %d\rfrom: %d\rcontent: %s", from, dest, "status_code: 400\n");
            }
            pthread_mutex_lock(&pipe_mutex);        // lock the pipe mutex to write into sub2for pipe
            pthread_cond_signal(&pipe_cond);        // wake up forwarder
            write(sub2for[1], internal_send_buffer, strlen(internal_send_buffer));        // write into the pipe
            pthread_mutex_unlock(&pipe_mutex);

            pthread_mutex_lock(&destination_mutex);
            destination = -1;
            pthread_mutex_unlock(&destination_mutex);
        }
        else{
            pthread_mutex_unlock(&destination_mutex);
            sleep(1);
        }
    }
}

void* SubThread(void* raw_comfd){
    int comfd = (int)raw_comfd;
    pthread_t sub_sub_thread_id;
    pthread_t self_thread_id = pthread_self();

    struct tmp_struct tmp;
    tmp.thread_id = self_thread_id;
    tmp.comfd = comfd;

    pthread_create(&sub_sub_thread_id, NULL, SubSubThread, (void*)&tmp);

    char recv_buffer[MAX_LENGTH];   // a buffer used to store the income data
    Reset(recv_buffer);
    while(1){
        int tmp_arr[] = {comfd, sockfd};
        ReceiveData(recv_buffer, comfd, tmp_arr, 2);
        // printf("recv:\n");
        // RawPrint(recv_buffer);
        // printf("end\n");
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
                int third_fd = TransferMsg_2(recv_buffer);

                if(third_fd == -1){
                    BadRequest(comfd);
                }
                /*
                if(third_fd != -1){
                    pthread_mutex_lock(&destination_mutex);
                    while(destination != (int)pthread_self()){
                        pthread_mutex_unlock(&destination_mutex);
                        sleep(1);       // wait until data sent to this subthread
                        pthread_mutex_lock(&destination_mutex);
                    }
                    pthread_mutex_unlock(&destination_mutex);

                    pthread_mutex_lock(&forwarder_mutex);
                    pthread_cond_signal(&forwarder_cond);
                    pthread_mutex_unlock(&forwarder_mutex);
                    char buffer[MAX_LENGTH];
                    Reset(buffer);
                    char* ptr = buffer;
                    char content[MAX_LENGTH];
                    while(buffer[strlen(buffer) - 1] != '\n'){
                        int length = read(for2sub[0], ptr, MAX_LENGTH);
                        if(length == -1){
                            continue;
                        }
                        ptr += length;
                    }
                    int tmp1, tmp2;
                    sscanf(buffer, "dest: %d\rfrom: %d\rcontent: %[^\n]s", &tmp1, &tmp2, content);
                    TransferMsg_3(comfd, content);
                }
                else{
                    BadRequest(comfd);
                }
                */


                break;
            case 5:
                pthread_mutex_lock(&third_party_mutex);
                third_party_response = 1;
                pthread_cond_signal(&third_party_cond);
                pthread_mutex_unlock(&third_party_mutex);
                break;
            case 6:
                pthread_mutex_lock(&third_party_mutex);
                third_party_response = 0;
                pthread_cond_signal(&third_party_cond);
                pthread_mutex_unlock(&third_party_mutex);
                break;
        }
        Reset(recv_buffer);
    }
}

int main() {	
    signal(SIGINT, signal_callback_handler);
    // setup forwarder
    pthread_t forwarder_thread_id;
    pthread_create(&forwarder_thread_id, NULL, Forwarder, NULL);
    struct sockaddr_in serverAddr, clientAddr;
    int iClientSize;
    // connection records are stored in a linked list
    // initialize the linked list

    pthread_mutex_lock(&connection_log_mutex);

    llhead = (struct connection_log*)malloc(sizeof(struct connection_log));
    llhead->next = NULL;
    llhead->num = 0;

    pthread_mutex_unlock(&connection_log_mutex);


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
        int rc = pthread_create(&thread_id, NULL, SubThread, (void*)comfd);
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
