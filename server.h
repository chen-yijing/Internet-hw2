#ifndef __SERVER_H__
#define __SERVER_H__
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<errno.h>
#include<unistd.h>
#include<netdb.h>
#include<signal.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#define BACKLOG 50
#define PORT 8002

char playername[10][16]={"STDIN","STDOUT","STDERR","Socket","apple","banana","Cake", "dog","egg","fish"};
int client_number;
int serverSocket;
int winmap[8]={146,292,584,14,112,896,546,168};		//total 8 methods(represented by binary code) to win OX chess 
short online[10];									//	ex:14=>00001110=>bit 1-2-3
short playing[10];
short mark[10];
pthread_mutex_t online_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t in_game_mutex=PTHREAD_MUTEX_INITIALIZER;
struct match{
	char map[3][8];
	int player1;
	int player2;
	int count;
}gamemap[10][10];

void *gamemenu();

#endif													
