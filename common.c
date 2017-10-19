/*
 * common.c
 * Copyright (C) 2017 weihao <weihao@weihao-PC>
 *
 * Distributed under terms of the MIT license.
 */

#include "common.h"

// used to indicate which function the client is requesting for
// 0: error, 1: get time, 2: get name, 3: get list, 4: send msg
char* function_code[4] = { "1\n", "2\n", "3\n", "4\n" };

void RawPrint(char* text){
    int length = strlen(text);
    for(int i = 0; i < length; i++){
        switch (text[i]){
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            default:
                    printf("%c", text[i]);
                    break;
        }
    }
    printf("\n");
}

// used to clear the receive buffer and reset ptr for a new cycle
void Reset(char* recv_buffer){
    for(int i = 0; i < MAX_LENGTH; i++){
        recv_buffer[i] = 0;
    }
    recv_buffer[0] = '\r';
}

void ReceiveData(char* recv_buffer, int recv_fd, int* close_fd, int amount){
    char* ptr = recv_buffer;
    int ret;
    // a complete message always ends with a character '\n'
	while(recv_buffer[strlen(recv_buffer) - 1] != '\n') {
		// receive data
		ret = recv(recv_fd, ptr, MAX_LENGTH, 0);
        // if some error happens, exit
		if(ret < 0) {
            printf("%d\n", ret);
			printf("recv() failed!\n");
            for(int i = 0; i < amount; i++){
                close(close_fd[i]);
            }
			exit(-1);
		}
        // if the peer disconnect
        if(ret == 0){
            Disconnect(recv_fd);
            return;
        }
		ptr = (char *)ptr + ret;
	}
}

void SendData(int send_fd, char* send_buffer){
    if(send(send_fd, send_buffer, strlen(send_buffer), 0) == -1) {
        printf("send() failed!\n");
        close(send_fd);
        exit(-1);
    }
}
