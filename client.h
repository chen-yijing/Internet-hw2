#ifndef __CLIENT_H__
#define __CLIENT_H__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#define SERVERPORT 8002

int sock;
int threadend = 0;
int data_flag = 0;
int match_flag = 0;
int nextturn = 0;
char data[128];
char playername[128];
char username[128];
pthread_mutex_t data_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t match_mutex=PTHREAD_MUTEX_INITIALIZER;

void *sendsock(void *arg);
void *recvsock(void *arg);
void playing(int socket);

#endif