#include "client.h"

static void sig_handler(int sig){
	if(sig==SIGINT){
		printf("\n\t***quit the game***\n\n");
		write(sock, "quit", sizeof("quit"));
		close(sock);
		exit(0);
	}
}

int main(){
	int connectok;
	struct sockaddr_in serverAddress;
	const char *serverIP = "127.0.0.1";
	unsigned short serverPort = SERVERPORT;
	struct sigaction sa;

	if((sock = socket(AF_INET, SOCK_STREAM, 0))<0){
		fprintf(stderr, "Error: socket\n");
		exit(EXIT_FAILURE);
	}

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);
	serverAddress.sin_port = htons(serverPort);

	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if(sigaction(SIGINT, &sa, NULL)==-1){
		fprintf(stderr, "Error: sigaction\n");
		exit(EXIT_FAILURE);
	}

	if(connectok = connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress))<0){
		fprintf(stderr, "Error: connect\n");
		exit(EXIT_FAILURE);
	}

	pthread_t recvsock_t, sendsock_t;
	pthread_create(&recvsock_t, NULL, recvsock, (void*)&sock);
	pthread_create(&sendsock_t, NULL, sendsock, (void*)&sock);
	pthread_join(recvsock_t, NULL);

	pthread_mutex_destroy(&data_mutex);

	close(sock);
	return 0;
}

void *recvsock(void *arg){
	int sock = *(int*)arg;
	char *temp;
	int n;
	while(1){
		pthread_mutex_lock(&data_mutex);
		n = read(sock, data, sizeof(data));
		if(strcmp(data, "quit")==0) break;
		else if(strncmp(data, "Invite", 6)==0){
			write(fileno(stdout), "\n", sizeof("\n"));
			write(fileno(stdout), data, sizeof(data));		//printf XXX invites you (Y/N)?
			temp = strtok(data, " ");
			temp = strtok(NULL, " ");
			pthread_mutex_lock(&match_mutex);
			strcpy(playername, temp);		//record match name
			match_flag=1;
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
		}
		else if(strncmp(data, "Start", 5)==0){
			match_flag = 1;
			for(int i=0; i<40; i++) printf("*");
			printf("\n");
			temp = strtok(data, ";");
			printf("\t%s\n", temp);		//print "Start Game\nO :...X:...
			temp = strtok(NULL, ";");	//temp = who turn
			nextturn=0;
			if(strcmp(temp, username)==0) nextturn=1;
			temp = strtok(NULL, ";");
			printf("%s\n", temp);		//print game map "_ _ _\n_ _ _\n..."
		}
		else if(strncmp(data, "Reject", 6)==0){
			printf("%s\n\n", data);		//print "Reject!!""
		}
		else if(strcmp(data, "Win")==0){
			printf("\n\t*** You Win !! Congratulation !! ***\n\n");
		}
		else if(strcmp(data, "Lose")==0){
			printf("\n\t*** Yoe Lose !! Keep your chin up !! ***\n\n");
		}
		else if(strncmp(data, "Even", 4)==0){
			printf("\n\t*** The game ended in a tie !! ***\n\n");
		}
		else if(strncmp(data, "Leave", 5)==0){
			temp = strtok(data, ";");
			temp = strtok(NULL, ";");
			printf("%s\n", temp);		//print "XXX Leave the room !!"
		}
		else if(strncmp(data, "username", 8)==0){
			temp = strtok(data, " ");
			temp = strtok(NULL, " ");
			memset(username, 0, sizeof(temp));
			strcpy(username, temp);		//record my name (playername[clientSocket]) which is setted by order of clientsockets
		}
		else if(strncmp(data, "Busy", 4)==0){
			printf("%s\n", data);		//print  "XXX is in game now !! Please choose another player !!\n"
		}
		else if(strcmp(data, "error")==0){
			printf("\n\t*** Incorrect player name !! Please try again !!\n\n");
		}
		else if(strcmp(data, "Error")==0){
			printf("\n\t*** Incorrect input !! Please try again !!\n\n"); 
		}

		pthread_mutex_unlock(&data_mutex);
		usleep(100000);
	}
}

void *sendsock(void *arg){
	int sock = *(int*)arg;
	char command[128];
	char response[128];
	fd_set readset;
	int flags;
	int n;
	int maxfdp;
	FILE *fp = stdin;
	pthread_detach(pthread_self());		//将状态改为unjoinable状态，确保资源的释放

	printf("\n----------------Welcome to the OX Chess Game-----------------\n");
	printf("\tHere is the Game Menu\n");
	printf("(menu) Display the command you can use\n");
	printf("(list) Display the players who are online\n");
	printf("(match) Create the new room\n");
	printf("(quit) Exit the game\n");
	printf("------------------------------------------------------------------\n");

	while(1){
		printf("## Command : ");
		scanf("%s", command);

		if(strcmp(command, "quit")==0){
			write(sock, command, sizeof(command));
			printf("\n\n\t*** Exit the game ***\n\tHave a nice day!!\n");
			break;
		}
		else if(strcmp(command, "list")==0){
			write(sock, command, sizeof(command));
			pthread_mutex_lock(&data_mutex);
			data[strlen(data)-1]='\0';
			printf("\n-------player(s) online--------\n");	
			printf("%s\n\n\n", data);
			pthread_mutex_unlock(&data_mutex);
			usleep(1000);
		}
		else if(strcmp(command, "menu")==0){
			printf("\n\t***Here is the Game Menu***\n");
			printf("(menu) Display the command you can use\n");
			printf("(list) Display the players who are online\n");
			printf("(match) Create the new room\n");
			printf("(quit) Exit the game\n\n");
		}
		else if(strcmp(command, "match")==0){
			printf("Choose the one player you want to invite : ");
			scanf("%s", playername);
			strcat(command, " ");
			strcat(command, playername);
			write(sock, command, sizeof(command));
			printf("\t\n Wait !!\n");
			pthread_mutex_lock(&data_mutex);
			pthread_mutex_unlock(&data_mutex);
			usleep(1000);
			if(match_flag) playing(sock);	//match success
			match_flag = 0;
		}
		else if(strcmp(command, "Y")==0||strcmp(command, "y")==0){
			int enterflag = 0;
			pthread_mutex_lock(&match_mutex);
			if(match_flag){		//if someone have invited => match success
				memset(response, 0, sizeof(response));
				strcpy(response, "Accept ");
				strcat(response, playername);
				write(sock, response, sizeof(response));
				enterflag = 1;
			}
			else printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
			
			if(enterflag) playing(sock);
			match_flag = 0;
		}
		else if(strcmp(command, "N")==0||strcmp(command, "n")==0){
			pthread_mutex_lock(&match_mutex);
			if(match_flag){		//if someone have invited => Reject
				memset(response, 0, sizeof(response));	
				strcpy(response, "Reject ");
				strcat(response, playername);
				write(sock, response, sizeof(response));
			}
			else printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
			match_flag = 0;
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
		}
		else{
			printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
		}
	}
}

void playing(int socket){
	int next;
	char temp[2];
	char response[128];

	while(1){
		pthread_mutex_lock(&data_mutex);
		if(strcmp(data, "quit")==0 || strcmp(data, "Win")==0 || strcmp(data, "Lose")==0 || strncmp(data, "Even", 4)==0 || strncmp(data, "Leave", 5)==0){
			pthread_mutex_unlock(&data_mutex);
			break;		//game end!!
		}
		else if(nextturn){
			while(1){
				printf("You turn (-1 is quit) : ");
				scanf("%d", &next);
				if(next>=1 && next<=9 || next==-1) break;
				else printf("*** Incorrect input !! Please enter the correct number(from  1 to 9) !!\n\n");
			}
			if(next==-1){
				printf("\n\t*** You leave the game !! ***\n");
				memset(response, 0, sizeof(response));
				strcpy(response, "Leave");
				write(socket, response, sizeof(response));
				pthread_mutex_unlock(&data_mutex);
				break;
			}
			memset(response, 0, sizeof(response));
			temp[0]=next+48;	//int => ascii (char)
			temp[1]='\0';
			strcpy(response, "Next;");
			strcat(response, temp);
			write(socket, response, sizeof(response));
		}
		pthread_mutex_unlock(&data_mutex);
		usleep(1000);
	}
}