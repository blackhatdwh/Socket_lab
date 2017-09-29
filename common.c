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
void *ptr;

// used to clear the receive buffer and reset ptr for a new cycle
void Reset(char* recv_buffer){
    for(int i = 0; i < MAX_LENGTH; i++){
        recv_buffer[i] = 0;
    }
    ptr = recv_buffer;
}

void ReceiveData(char* recv_buffer, int recv_fd, int* close_fd, int amount){
    int count = -1;
    int ret;
    // write the received data into recv_buffer
	ptr = recv_buffer;
    ret = recv(recv_fd, ptr, MAX_LENGTH, 0);
    ptr = (char *)ptr + ret;
    count += ret;
    if(count == -1){
        return;
    }
    // a complete message always ends with a character '\n'
	while(recv_buffer[count] != '\n') {
		// receive data
		ret = recv(recv_fd, ptr, MAX_LENGTH, 0);
        count += ret;
        // if some error happens, exit
		if(ret <= 0) {
			printf("recv() failed!\n");
            for(int i = 0; i < amount; i++){
                close(close_fd[i]);
            }
			exit(-1);
		}
		ptr = (char *)ptr + ret;
	}
}
