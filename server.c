#include "server.h"

static void sig_handler(int sig){
	if(sig==SIGINT){
		for(int i=0; i<10; i++){
			if(online[i]==1) 	write(i, "quit", sizeof("quit"));
		}
		usleep(1000);
		close(serverSocket);
		exit(0);
	}
}

int main(){
	int yes;
	int clientSocket;
	struct sockaddr_in serverAddress;
	int listening;
	struct sigaction sa;

	if((serverSocket = socket(AF_INET, SOCK_STREAM, 0))<0){
		fprintf(stderr, "Error: socket\n");
		exit(EXIT_FAILURE);
	}
	
	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
		fprintf(stderr, "Error: setsockopt\n");
		exit(EXIT_FAILURE);
	}

	if(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress))<0){
		fprintf(stderr, "Error: bind\n");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if(sigaction(SIGINT, &sa, NULL)==-1){
		fprintf(stderr, "Error: sigaction\n");
		exit(EXIT_FAILURE);
	}

	if((listening = listen(serverSocket, BACKLOG))<0){
		fprintf(stderr, "Error: listen\n");
		exit(EXIT_FAILURE);
	}

	pthread_t t;
	pthread_attr_t attr;
	int count=0;
	
	memset(playing, -1, sizeof(playing));
	memset(online, 0, sizeof(online));
	memset(mark, -1, sizeof(mark));
	pthread_attr_init(&attr);                                       
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);		// thread detached

	while(1){
		clientSocket = accept(serverSocket, NULL, NULL);
		pthread_create(&t, &attr, gamemenu, (void *)&clientSocket);
		pthread_mutex_lock(&online_mutex);
		online[clientSocket] = 1;
		pthread_mutex_unlock(&online_mutex);
	}
	pthread_mutex_destroy(&online_mutex);
	close(serverSocket);

	return 0;
}

void *gamemenu(void *arg){
	char buffer[128];
	char response[128];
	char list[128];
	char *player;
	char *temp;
	int player_sock;
	char data[128];
	char resplayer1[128];
	char resplayer2[128];
	char username[128];
	
	int clientSocket = *(int *)arg;

	printf("%d\n", clientSocket);
	memset(username, 0, sizeof(username));
	strcpy(username, "username: ");
	strcat(username, playername[clientSocket]);
	strcat(username, " ");
	write(clientSocket, username, sizeof(username));		//send back player's name

	while(1){
		memset(buffer, 0, sizeof(buffer));
		read(clientSocket, buffer, sizeof(buffer));
		if(strcmp(buffer, "quit")==0){
			write(clientSocket, buffer, sizeof(buffer));   // inform the recvthread of the client process to close
			pthread_mutex_lock(&in_game_mutex);
			if(playing[clientSocket]>=0){					//告訴對手我已經quit game了
				memset(response, 0, sizeof(response));
				strcpy(response, "Leave;");
				strcat(response, "\t\n*** ");
	            strcat(response, playername[clientSocket]);
        	    strcat(response, " Leave the room !! ***\n;");

				write(playing[clientSocket], response, sizeof(response));
				playing[playing[clientSocket]]=-1;
				playing[clientSocket]=-1;
			}
			pthread_mutex_unlock(&in_game_mutex);
			usleep(1000);
			break;
		}
		else if(strcmp(buffer, "list")==0){			//send back who is online
			memset(list, 0, sizeof(list));
			for(int i=0; i<10;i++){
				if(online[i]){
					strcat(list, playername[i]);
					if(i==clientSocket) strcat(list, "(you)");
					if(playing[i]>0){
						strcat(list, "\t(Playing with ");
						strcat(list, playername[playing[i]]);
						strcat(list, ")");
					}
					strcat(list, "\n");
				}
			}
			write(clientSocket, list, sizeof(list));
		}
		else if(strncmp(buffer, "Reject", 6)==0){
			temp = strtok(buffer, " ");
			temp = strtok(NULL, " ");				//temp = 被reject的玩家名			
			player_sock = -1;
			for(int i=0; i<10; i++){
				if(strcmp(temp, playername[i])==0){
					player_sock = i;
					break;
				}
			}
			memset(response, 0, sizeof(response));
			write(player_sock, "Reject !!", sizeof("Reject !!"));
		}
		else if(strncmp(buffer, "Accept", 6)==0){
			temp = strtok(buffer, " ");
			temp = strtok(NULL, " ");
			player_sock = -1;
			for(int i=0; i<10; i++){
				if(strcmp(temp, playername[i])==0){
					player_sock = i;
					break;
				}
			}
			pthread_mutex_lock(&in_game_mutex);
			playing[clientSocket] = player_sock;		//record who I'm playing with now
			playing[player_sock] = clientSocket;
			pthread_mutex_unlock(&in_game_mutex);
			usleep(1000);
		
			memset(resplayer1, 0, sizeof(resplayer1));
			memset(resplayer2, 0, sizeof(resplayer2));

			strcpy(resplayer1, "Start Game\n");
			strcat(resplayer1, "O : ");
			strcat(resplayer1, playername[clientSocket]);
			strcat(resplayer1, "(you)\t");
			strcat(resplayer1, "X : ");
			strcat(resplayer1, playername[player_sock]);

			strcpy(resplayer2, "Start Game\n");
			strcat(resplayer2, "O : ");
			strcat(resplayer2, playername[clientSocket]);
			strcat(resplayer2, "\tX : ");
			strcat(resplayer2, playername[player_sock]);
			strcat(resplayer2, "(you)");

			strcat(resplayer1, ";");
			strcat(resplayer1, playername[clientSocket]);    // who turn
			strcat(resplayer1, ";");

			strcat(resplayer2, ";");
			strcat(resplayer2, playername[clientSocket]);    // who turn
			strcat(resplayer2, ";");

			mark[clientSocket] = 0;		//mark:O
			mark[player_sock] = 1;		//mark:X

			int minsocket = clientSocket > player_sock ? player_sock : clientSocket;	//minsocket = min(client,player)
			int maxsocket = clientSocket > player_sock ? clientSocket : player_sock;	//maxsocket = max(client,player)

			//initialize gamemap
			for(int i=0; i<3; i++){
				for(int j=0; j<3; j++){
					gamemap[minsocket][maxsocket].map[i][j*2]='_';
					gamemap[minsocket][maxsocket].map[i][j*2+1]=' ';
					gamemap[minsocket][maxsocket].player1=0;
					gamemap[minsocket][maxsocket].player2=0;
				}
				gamemap[minsocket][maxsocket].map[i][6]='\n';
				gamemap[minsocket][maxsocket].map[i][7]='\0';
				strcat(resplayer1, gamemap[minsocket][maxsocket].map[i]);
				strcat(resplayer2, gamemap[minsocket][maxsocket].map[i]);
			}
			gamemap[minsocket][maxsocket].count = 0;
			strcat(resplayer1, ";");
			strcat(resplayer2, ";");

			write(clientSocket, resplayer1, sizeof(resplayer1));
			write(player_sock, resplayer2, sizeof(resplayer2));
		}
		else if(strncmp(buffer, "match", 5)==0){
			player=strtok(buffer, " ");
			player=strtok(NULL, " ");
			player_sock=-1;
			for(int i=0; i<10; i++){
				if(strcmp(player, playername[i])==0 && online[i]){
					player_sock=i;
					break;
				}
			}
			if(player_sock<0){		
				memset(response, 0, sizeof(response));
				strcpy(response, "error");
				write(clientSocket, response, sizeof(response));
			}
			else{
				pthread_mutex_lock(&in_game_mutex);
				if(playing[player_sock]<0){		//the player I requested is NOT playing with other right now => match SUCCESSFULLY!!!
					memset(data, 0, sizeof(data));
					printf("Successful !!\n");
					strcpy(data, "Invite: ");
					strcat(data, playername[clientSocket]);
					strcat(data, " invites you (Y/N) ? ");
					write(player_sock, data, sizeof(data));
					usleep(1000);
				}
				else{		//the player I requested is playing with other right now => choose again
					memset(response, 0, sizeof(response));
					strcpy(response, "Busy : ");
					strcat(response, player);
					strcat(response, " is in game now !! Please choose another player !!\n");
					write(clientSocket, response, sizeof(response));
				}
				pthread_mutex_unlock(&in_game_mutex);
				usleep(1000);
			}
		}
		else if(strncmp(buffer, "Next", 4)==0){
			int location = -1;
			int minsocket = playing[clientSocket] > clientSocket ? clientSocket : playing[clientSocket];	//minsocket = min(client,playing)
            int maxsocket = playing[clientSocket] > clientSocket ? playing[clientSocket] : clientSocket;	//maxsocket = max(client,playing)
            char tempmark =  mark[clientSocket]==0 ? 'O' : 'X';		//0=>O, 1=>X
			int winflag = 0;
			char temp1;

			temp = strtok(buffer, ";");
			temp = strtok(NULL, ";");
			location = 1 << (temp[0] - 48);		//revert to binary code, location = 2^(temp[0]-48) (binary)
			switch(temp[0]){
                case '1':
                    temp1 = gamemap[minsocket][maxsocket].map[0][0];
                    break;
                case '2':
                    temp1 = gamemap[minsocket][maxsocket].map[0][2];
                    break;
                case '3':
                    temp1 = gamemap[minsocket][maxsocket].map[0][4];
                    break;
            	case '4':
                    temp1 = gamemap[minsocket][maxsocket].map[1][0];
                    break;
                case '5':
                    temp1 = gamemap[minsocket][maxsocket].map[1][2];
                    break;
            	case '6':
                    temp1 = gamemap[minsocket][maxsocket].map[1][4];
                    break;
                case '7':
                    temp1 = gamemap[minsocket][maxsocket].map[2][0];
                    break;
                case '8':
                    temp1 = gamemap[minsocket][maxsocket].map[2][2];
                    break;
                case '9':
                	temp1 = gamemap[minsocket][maxsocket].map[2][4];
                    break;
            }
			if(temp1!='_'){
				write(clientSocket, "Error", sizeof("Error"));
				continue;
			}
			++gamemap[minsocket][maxsocket].count;		//total step count in a game, range:0~9

			if(minsocket==clientSocket){
				gamemap[minsocket][maxsocket].player1 |= location;		//logic OR, player1 = player1 | location
				for(int i=0; i<8; i++){
					if(gamemap[minsocket][maxsocket].player1==winmap[i]){		
						winflag = 1;
						write(minsocket, "Win", sizeof("Win"));		//send result message to players(clients)
						write(maxsocket, "Lose", sizeof("Lose"));
						playing[minsocket]=playing[maxsocket]=-1;
						break;
					}
				}
			}
			else{
				gamemap[minsocket][maxsocket].player2 |= location;
				for(int i=0; i<8; i++){
					if(gamemap[minsocket][maxsocket].player2==winmap[i]){
						winflag = 1;
						write(minsocket, "Lose", sizeof("Lose"));
						write(maxsocket, "Win", sizeof("Win"));
						playing[minsocket]=playing[maxsocket]=-1;
						break;
					}
				}
			}
			if(winflag) continue;
			else if(gamemap[minsocket][maxsocket].count >= 9){		//九個位置都填完還是沒勝負=>平手
				memset(response, 0, sizeof(response));
				strcpy(response, "Even;The game ended in a tie !!;");
				write(minsocket, response, sizeof(response));		//send result to clients
				write(maxsocket, response, sizeof(response));
				playing[minsocket]=playing[maxsocket]=-1;		//reset
				continue;
			}

			memset(response, 0, sizeof(response));
			strcpy(response, "Start;");
			pthread_mutex_lock(&in_game_mutex);
			strcat(response, playername[playing[clientSocket]]);			//record who is next
			strcat(response, ";");

			switch(temp[0]){
				case '1':
					gamemap[minsocket][maxsocket].map[0][0] = tempmark;		//mark have setted before
					break;													//char tempmark =  mark[clientSocket]==0 ? 'O' : 'X';		
				case '2':													//0=>O, 1=>X
					gamemap[minsocket][maxsocket].map[0][2] = tempmark;
					break;
				case '3':
					gamemap[minsocket][maxsocket].map[0][4] = tempmark;
					break;
				case '4':
					gamemap[minsocket][maxsocket].map[1][0] = tempmark;
					break;
				case '5':
					gamemap[minsocket][maxsocket].map[1][2] = tempmark;
					break;
				case '6':
					gamemap[minsocket][maxsocket].map[1][4] = tempmark;
					break;
				case '7':
					gamemap[minsocket][maxsocket].map[2][0] = tempmark;
					break;
				case '8':
					gamemap[minsocket][maxsocket].map[2][2] = tempmark;
					break;
				case '9':
					gamemap[minsocket][maxsocket].map[2][4] = tempmark;
					break;
			}

			for(int i=0; i<3; i++){
                strcat(response, gamemap[minsocket][maxsocket].map[i]);		//game map
          	}
            strcat(response, ";");
			
			write(playing[clientSocket], response, sizeof(response));
			pthread_mutex_unlock(&in_game_mutex);
			write(clientSocket, response, sizeof(response));
		}
		else if(strcmp(buffer, "Leave")==0){
			int minsocket = playing[clientSocket] > clientSocket ? clientSocket : playing[clientSocket];
            int maxsocket = playing[clientSocket] > clientSocket ? playing[clientSocket] : clientSocket;

			memset(response, 0, sizeof(response));
			strcat(response, "Leave;");
			strcat(response, "\t\n*** ");
			strcat(response, playername[clientSocket]);
			strcat(response, " Leave the room !! ***\n;");
			write(playing[clientSocket], response, sizeof(response));		//傳給對手說你對手退出遊戲了	
			playing[minsocket]=playing[maxsocket]=-1;
		}
	}
	pthread_mutex_lock(&online_mutex);
	online[clientSocket]=0;			
	pthread_mutex_unlock(&online_mutex);

	close(clientSocket);
	return NULL; //pthread_exit(NULL);
}