/*
 * common.h
 * Copyright (C) 2017 weihao <weihao@weihao-PC>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef COMMON_H
#define COMMON_H
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
extern char* function_code[4];
extern void* ptr;
void Reset(char* recv_buffer);
void ReceiveData(char* recv_buffer, int recv_fd, int* close_fd, int amount);

#endif /* !COMMON_H */
