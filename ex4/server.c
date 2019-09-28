/*
 Authors: Dan Waisel 312490808 Yuval Tal 205704612
 Project: ex4
 Description: Online game of Connect Four. This project include Client and Server.
 */
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include "send_receive.h"
#include "constants.h"
#include "client.h"
// Constants -------------------------------------------------------------------

#define ERROR_CODE ((int)(-1))
#define SUCCESS_CODE ((int)(0))
#define NUM_OF_WORKER_THREADS 2
#define MAX_NUM_OF_PLAYERS 2
#define MAX_LOOPS 3
#define SEND_STR_SIZE 400
#define DRAW_DETECTED 3

// Variables -------------------------------------------------------------------
HANDLE ThreadHandles[NUM_OF_WORKER_THREADS];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS];
char* players_username_arr[MAX_NUM_OF_PLAYERS];
int game_started_server;
int notify_game_started;
int played_moves_arr[ROW_NUMBER_X_COL_NUM];
int turn_player_num;
int board_server[ROW_NUMBER][COL_NUMBER];
int game_ended;
int winner;
// Function Declarations -------------------------------------------------------
void GetUserNameFromMessage(char* username, char* message);
void InitializePlayMovesArr();
void InitializeBoardServer();
void CreateBoardViewMessage(char* SendStr);
static int FindFirstUnusedThreadSlot();
static DWORD ServiceThread(int Ind);
void CreateNewUserAcceptedMessage(char* SendStr);
void CreateNewUserDeclinedMessage(char* SendStr);
static int CountPlayers();
void CreateGameStartedMessage(char* SendStr);
void CreateTurnSwitchMessage(char* SendStr, int turn_player_num);
int LegalMove(char* AcceptedStr);
void UpdatePlayedMovesArr(char accepted_move);
void UpdateBoardServer(char accepted_move, int Ind);
void CreatePlayDeclinedMessage(char* SendStr, char* error_message);
void CreatePlayAcceptedMessage(char* SendStr);
void CreateGameEndedMessage(char* SendStr);
void CreateReceiveMessage(char* AcceptedStr, char* SendStr, int Ind);
void PrintServiceSocketErrorAndCloseSocket(int Ind);
void InitiallizeMainServer();
int IsGameEnded();


// Function Definitions -------------------------------------------------------

/*
functionality:1)	inits 2)	Open,bind,listen… 3)	Inifinite loop for accepting new clients  4)opening service thread
*/


void MainServer()
{
	
	int Ind;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	TransferResult_t SendRes;
	//initializations
	InitiallizeMainServer();
	// Initialize Winsock.
	WSADATA wsaData;
	winner = 0;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR)
	{
		printf("Custom message: error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		fprintf(p_log_file,"Custom message: error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return;
	}
	/* The WinSock DLL is acceptable. Proceed. */

	// Create a socket.    
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (MainSocket == INVALID_SOCKET)
	{
		printf("Custom message: Error at socket( ): %ld\n", WSAGetLastError());
		fprintf(p_log_file, "Custom message: Error at socket( ): %ld\n", WSAGetLastError());
		exit(1);
	}

	// Bind the socket.
	/*
	For a server to accept client connections, it must be bound to a network address within the system.
	The following code demonstrates how to bind a socket that has already been created to an IP address
	and port.
	Client applications use the IP address and port to connect to the host network.
	The sockaddr structure holds information regarding the address family, IP address, and port number.
	sockaddr_in is a subset of sockaddr and is used for IP version 4 applications.
	*/
	// Create a sockaddr_in object and set its values.
	// Declare variables

	Address = inet_addr(SERVER_ADDRESS_STR);
	if (Address == INADDR_NONE)
	{
		printf("Custom message: The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		fprintf(p_log_file, "Custom message: The string \"%s\" cannot be converted into an ip address. ending program.\n",
			SERVER_ADDRESS_STR);
		exit(1);
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;//ip adress of the socket
	service.sin_port = htons(server_port); //The htons function converts a u_short from host to TCP/IP network byte order 
										   //( which is big-endian ).
										   /*
										   The three lines following the declaration of sockaddr_in service are used to set up
										   the sockaddr structure:
										   AF_INET is the Internet address family.
										   "127.0.0.1" is the local IP address to which the socket will be bound.
										   2345 is the port number to which the socket will be bound.
										   */

										   // Call the bind function, passing the created socket and the sockaddr_in structure as parameters. 
										   // Check for general errors.
	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		fprintf( p_log_file, "bind( ) failed with error %ld. Ending program\n", WSAGetLastError()); 
		exit(1);
	}

	// Listen on the Socket.
	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		fprintf(p_log_file, "Failed listening on socket, error %ld.\n", WSAGetLastError());
		exit(1);
	}

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;

	printf("Waiting for a client to connect...\n");

	while(TRUE)
	{//MainSocket waits for new connections, AcceptsSockets -takes care of every new connection-creating a thread that gets the AcceptsSockets  as param
		SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET)
		{
			printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			fprintf(p_log_file, "Accepting connection with client failed, error %ld\n", WSAGetLastError());
			exit(1);
		}

		printf("Client Connected.\n");

		Ind = FindFirstUnusedThreadSlot();
		//third player is declined
		if (Ind == NUM_OF_WORKER_THREADS) //no slot is available
		{
			//----step5_Client-----
			SendRes = SendString("NEW_USER_DECLINED", AcceptSocket);
			if (SendRes == TRNS_FAILED)
			{
				printf("send() failed, error %d\n", WSAGetLastError());
				fprintf(p_log_file, "send() failed, error %d\n", WSAGetLastError());
				closesocket(AcceptSocket);
				exit (1);
			}
			shutdown(AcceptSocket, SD_SEND); //Closing the socket, dropping the connection.
			closesocket(AcceptSocket);
		}
		//first or second player- opening service threads and storing it and the corresponding socket in global array
		else
		{
			ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
											  // AcceptSocket, instead close 
											  // ThreadInputs[Ind] when the
											  // time comes.
			ThreadHandles[Ind] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)ServiceThread,
				Ind,
				0,
				NULL
			);
			if (ThreadHandles[Ind] == NULL)
			{
				printf("Custom message: Error in opening thread\n");
				fprintf(p_log_file, "Custom message: Error in opening thread\n");
				exit(1);
			}
		}
		
	} 

}

/*
[IN]: -
[OUT]: -
return value:int- index of the player
functionality: Finding Ind- index of socket&thread of a player in game
*/

static int FindFirstUnusedThreadSlot()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}
/*
[IN]: -
[OUT]: -
return value:int- current number of players 
functionality: going over the thread handles array (that the server hold for its clients) to check how many players(clients)are connected currently.
*/
static int CountPlayers()
{
	int Ind;
	int count = 0;
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			continue;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				continue;
			}
			count++;
		}
	}

	return count;
}

/*
functionality: a.	Endless loop until- to contact with clients
i.	recieving a message from client- AcceptedStr
ii.	get msg type
iii.	responds the client according to the msg type
	1)NEW_USER_REQUEST 
	2.	PLAY_REQUEST -  a.	Legal move: player's turn and suitable- Sending to client PLAY_ACCEPTED  b.	Illegal move: Sending to client PLAY_DECLINED
	3.	SEND_MESSAGE- create RECEIVE_MESSAGE and send only if there is another player connected.
	4.	other- don’t understand
	5.	checks if notify to start game: a.	sending GAME_STARTED , b.	creating and sending  BOARD_VIEW , c.	checking if game ended- if so sending GAME_ENDED , d.	creating and sending  TURN_SWITCH
*/
//Service thread is the thread that opens for each successful client connection and "talks" to the client.
static DWORD ServiceThread(int Ind)
{
	char board_message_string[MAX_MESSAGE_LEN];
	SOCKET* t_socket = &ThreadInputs[Ind];
	char message_type[MAX_MESSAGE_TYPE_LEN];
	char SendStr[SEND_STR_SIZE];
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	BOOL Done = FALSE;
	char player_username[MAX_USERNAME_LENGTH];
	char accepted_move;
	char error_message[MAX_ERROR_MESSAGE_LEN];
	

	while (!Done)
	{
		
		char *AcceptedStr = NULL;
		strcpy(SendStr,"");
		//recieving a message from client- AcceptedStr 
		RecvRes = ReceiveString(&AcceptedStr, *t_socket);

		if (RecvRes == TRNS_FAILED)
		{
			goto Restarting_server;
		}
		// if client exits the program we get here closing his socket 
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			printf("Player disconnected. Ending communication.\n");///log
			fprintf(p_log_file, "Player disconnected. Ending communication.\n");
			goto Restarting_server;
			//closesocket(*t_socket);
			//closesocket(ThreadInputs[1-Ind]);
			return ERROR_CODE;
		}

	//get msg type
		GetMessageType(AcceptedStr, message_type);
		
	//responds the client according to the msg type

		//1)If got NEW_USER_REQUEST 
		if (STRINGS_ARE_EQUAL(message_type, "NEW_USER_REQUEST"))
		{
			
			//extracts username from message 
			GetUserNameFromMessage(player_username, AcceptedStr);
			//if username taken - creating NEW_USER_DECLINED msg
			if (strcmp(players_username_arr[1 - Ind], player_username) == 0) 
			{
				CreateNewUserDeclinedMessage(SendStr);
				//Closing the connection of the other player as well(current player's connection is closed later)
				SendRes = SendString(SendStr, ThreadInputs[Ind]);
				//SendRes = SendString(SendStr, ThreadInputs[1-Ind]);
				shutdown(*t_socket, SD_SEND);
				shutdown(ThreadInputs[1 - Ind], SD_SEND);
				continue;
			}
			//if username free: filling players_username_arr, creating NEW_USER_ACCEPTED msg,
			else
			{
				printf("player %d's name is %s\n", Ind + 1, player_username);

				strcpy((players_username_arr[Ind]),player_username);
				CreateNewUserAcceptedMessage(SendStr);
				//if 2 players in - notify to start game
				if ((strcmp(players_username_arr[0], "") != 0) && (strcmp(players_username_arr[1], "") != 0))
				{
					game_started_server = 1;
					notify_game_started = 1;
				}
			}
			
		}
		//2)if got a PLAY_REQUEST message
		else if (STRINGS_ARE_EQUAL(message_type, "PLAY_REQUEST"))
		{	//a)if game not started- let the user know
			if (game_started_server == 0)
			{
				strcpy(error_message, "Game has not started");
				CreatePlayDeclinedMessage(SendStr, error_message);
			}
			//b)if player's turn
			else if(Ind == turn_player_num)
			{
				//if the move is legal- update board and send PLAY_ACCEPTED  to client and goto Send_Board_View
				if (LegalMove(AcceptedStr)==TRUE)
				{
					accepted_move = AcceptedStr[13];
					UpdateBoardServer(accepted_move, Ind);
					UpdatePlayedMovesArr(accepted_move);
					turn_player_num = 1 - turn_player_num;
					//Message to be sent -filling SendStr =PLAY_ACCEPTED 
					CreatePlayAcceptedMessage(SendStr);
					//send the response
					SendRes = SendString(SendStr, *t_socket);

					if (SendRes == TRNS_FAILED)
					{
						PrintServiceSocketErrorAndCloseSocket(Ind);
						exit(1);
					}

					game_ended = IsGameEnded();
					if (game_ended == TRUE)
					{
						winner = Ind+1;
					}
					else if (game_ended == DRAW_DETECTED)
					{
						winner = DRAW_DETECTED;
					}
					goto Send_Board_View;
				}
				else
				{
					strcpy(error_message, "Illegal move");
					CreatePlayDeclinedMessage(SendStr, error_message);
				}
			}
			//c)if not player's turn- cant send play request
			else
			{
				strcpy(error_message, "Not your turn");
				CreatePlayDeclinedMessage(SendStr,error_message);
				
			}
		}
		//3) if got SEND_MESSAGE
		else if (STRINGS_ARE_EQUAL(message_type, "SEND_MESSAGE"))
		{
			//create RECEIVE_MESSAGE and send only if there is another player connected.
			CreateReceiveMessage(AcceptedStr, SendStr,Ind);
			if (ThreadInputs[1 - Ind] != NULL )
			{
				if (strcmp(players_username_arr[1-Ind],"")!=0)
				{
					SendRes = SendString(SendStr, ThreadInputs[1 - Ind]);
					if (SendRes == TRNS_FAILED)
					{
						PrintServiceSocketErrorAndCloseSocket(Ind);
						exit(1);
					}
				}
			}
			
			continue;
		}
		//4)
		else
		{
			strcpy(SendStr, "I don't understand");
		}

		//send the response
		SendRes = SendString(SendStr, *t_socket);

		if (SendRes == TRNS_FAILED)
		{
			PrintServiceSocketErrorAndCloseSocket(Ind);
			exit(1);
		}

		free(AcceptedStr);
		//5)	response only once when game started- creating and sendinh GAME_STARTED msg, updating notify_game_started to not enter here again.
		if (notify_game_started)
		{
			notify_game_started = 0;
			CreateGameStartedMessage(SendStr);
		//a)sending GAME_STARTED to both players
			//sending GAME_STARTED to the 1st player
			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				PrintServiceSocketErrorAndCloseSocket(Ind);
				exit(1);
			}
			//sending GAME_STARTED to the 2nd player
			SendRes = SendString(SendStr, ThreadInputs[1-Ind]);

			if (SendRes == TRNS_FAILED)
			{
				PrintServiceSocketErrorAndCloseSocket(Ind);
				exit(1);
			}
//we get here in the beginning of the game or if playing legal move			
Send_Board_View:
		//b)creating BOARD_VIEW message and sending it to 2 players
			CreateBoardViewMessage(board_message_string);
			//sending BOARD_VIEW to the 1st player
			strcpy(SendStr, board_message_string);
			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				PrintServiceSocketErrorAndCloseSocket(Ind);
				exit(1);
			}
			//sending BOARD_VIEW to the 2nd player
			SendRes = SendString(SendStr, ThreadInputs[1 - Ind]);

			if (SendRes == TRNS_FAILED)
			{
				PrintServiceSocketErrorAndCloseSocket(Ind);
				exit(1);
			}

		//c)checking if game ended-if so sending GAME_ENDED and shut down both player's sockets and go out of the loop
			if (game_ended==TRUE || game_ended == DRAW_DETECTED)
			{
				
				CreateGameEndedMessage(SendStr);
				SendRes = SendString(SendStr, *t_socket);
				
				if (SendRes == TRNS_FAILED)
				{
					PrintServiceSocketErrorAndCloseSocket(Ind);
					exit(1);
				}
				shutdown(*t_socket, SD_SEND);
				SendRes = SendString(SendStr, ThreadInputs[1-Ind]);

				if (SendRes == TRNS_FAILED)
				{
					PrintServiceSocketErrorAndCloseSocket(Ind);
					exit(1);
				}
				shutdown(ThreadInputs[1 - Ind], SD_SEND);
				continue;
			
			}

		//d)creating TURN_SWITCH messgge  and sending it to two players
			CreateTurnSwitchMessage(SendStr,turn_player_num);
			
			SendRes = SendString(SendStr, *t_socket);

			if (SendRes == TRNS_FAILED)
			{
				PrintServiceSocketErrorAndCloseSocket(Ind);
				exit(1);
			}
			//sending BOARD_VIEW to the 2nd player
			SendRes = SendString(SendStr, ThreadInputs[1 - Ind]);

			if (SendRes == TRNS_FAILED)
			{
				PrintServiceSocketErrorAndCloseSocket(Ind);
				exit(1);
			}

		}

	}
	/*we can here in the cases:
	1- if player exits, the other player will get here and make the restarts.
	2- if the player closes the console window- he gets here
	*/
Restarting_server:
	//shutdown(*t_socket, SD_SEND);
	closesocket(*t_socket);
	shutdown(ThreadInputs[1 - Ind], SD_SEND);
	//shutdown(ThreadInputs[1 - Ind], SD_SEND);
	//closesocket(ThreadInputs[1 - Ind]);
	strcpy(players_username_arr[Ind], "");
	game_started_server = 0;
	strcpy(players_username_arr[0], "");
	strcpy(players_username_arr[1], "");
	game_started_server = 0;
	notify_game_started = 0;
	turn_player_num = 0;
	game_ended = FALSE;
	winner = 0;
	InitializePlayMovesArr();
	InitializeBoardServer();
	printf("Waiting for a client to connect...\n");
	return ERROR_CODE;

}

/*
[IN]: -
[OUT]: string - a NEW_USER_ACCEPTED message
return value:-
functionality: creates NEW_USER_ACCEPTED message in format NEW_USER_ACCEPTED:num_of_players\0 and saves it in output param.
*/
void CreateNewUserAcceptedMessage(char* SendStr)
{
	int num_of_players;	
	num_of_players = CountPlayers();
	strcpy(SendStr, "NEW_USER_ACCEPTED:");
	SendStr[18] = '0' + num_of_players;
	SendStr[19] = '\0';
}
/*
[IN]: -
[OUT]: string - a NEW_USER_DECLINED message
return value:-
functionality: creates NEW_USER_DECLINED message in format NEW_USER_DECLINED\0 and saves it in output param.
*/
void CreateNewUserDeclinedMessage(char* SendStr)
{
	strcpy(SendStr, "NEW_USER_DECLINED");
}
/*
[IN]: -
[OUT]: string - a NEW_USER_DECLINED message
return value:-
functionality: creates NEW_USER_DECLINED message in format NEW_USER_DECLINED\0 and saves it in output param.
*/
void CreateGameStartedMessage(char* SendStr)
{
	strcpy(SendStr, "GAME_STARTED");
}





void GetUserNameFromMessage(char* username,char* message)
{
	int i = 0;
	int j = 0;
	while (message[i]!=':')
	{
		i++;
	}
	i++;
	while (message[i] != '\0')
	{
		username[j] = message[i];
		i++;
		j++;
	}
	username[j] = '\0';
}

//initialize play_move_arr
void InitializePlayMovesArr()
{
	int i;
	for (i = 0; i < ROW_NUMBER_X_COL_NUM; i++)
	{
		played_moves_arr [i]=-1 ;
	}
}
//initialize board_server
void InitializeBoardServer()
{
	int i;
	int j;
	for (i = 0; i < ROW_NUMBER; i++)
	{
		for (j = 0; j < COL_NUMBER; j++)
		{
			board_server[i][j] = 0;
		}
	}
}

/*
[IN]: -
[OUT]: string - board view msg
return value:-
functionality: 
*/

void CreateBoardViewMessage(char* SendStr)
{
	strcpy(SendStr, "BOARD_VIEW:");
	int i;
	int j = 11;
	for (i = 0; i < ROW_NUMBER_X_COL_NUM; i++)
	{
		if (played_moves_arr[i] == -1)
		{
			break;
		}
		SendStr[j] =( '0'+ played_moves_arr[i]);
		SendStr[j + 1] = ';';
		j+=2;
	}
	SendStr[j - 1] = '\0';

}
/*
[IN]: -turn player num
[OUT]: string - turn switch msg
return value:-
functionality:
*/
void CreateTurnSwitchMessage(char* SendStr, int turn_player_num)
{
	strcpy(SendStr, "TURN_SWITCH:");
	strcat(SendStr, players_username_arr[turn_player_num]);
}

/*
[IN]: -
[OUT]: string - PLAY_ACCEPTED msg
return value:-
functionality:
*/
void CreatePlayAcceptedMessage(char* SendStr)
{
	strcpy(SendStr, "PLAY_ACCEPTED");
}
/*
[IN]: -
[OUT]: string - PLAY_DECLINED msg
return value:-
functionality:
*/
void CreatePlayDeclinedMessage(char* SendStr,char* error_message)
{
	strcpy(SendStr, "PLAY_DECLINED:");
	strcat(SendStr, error_message);
}

/*
[IN]: string - AcceptedStr
[OUT]:-
return value:-legal: return TRUE else FALSE
functionality: gets PLAY_REQUEST msg from user and determines if the move is legal
*/

int LegalMove(char* AcceptedStr)
{
	if (strlen(AcceptedStr)!=14)
	{
		return FALSE;
	}
	if (AcceptedStr[13] != '0' && AcceptedStr[13] != '1' && AcceptedStr[13] != '2' && AcceptedStr[13] != '3' && AcceptedStr[13] != '4' && AcceptedStr[13] != '5' && AcceptedStr[13] != '6')
	{
		return FALSE;
	}
	if (board_server[ROW_NUMBER - 1][AcceptedStr[13] - '0'] != 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
// adds a disk to the server's  game board 
void UpdateBoardServer(char accepted_move,int Ind)
{
	int i;
	for (i = 0; i < ROW_NUMBER; i++)
	{
		if (board_server[i][accepted_move - '0']==0)
		{
			board_server[i][accepted_move - '0'] = Ind+1;
			break;
		}
	}
}

//adds the last move to the played moves array
void UpdatePlayedMovesArr(char accepted_move)
{
	int i;
	for (i = 0; i < ROW_NUMBER_X_COL_NUM; i++)
	{
		if (played_moves_arr[i] == -1)
		{
			played_moves_arr[i] = (accepted_move - '0');
			break;
		}
	}
}

// checks if game ended-somebody won TRUE
// returns DRAW_DETECTED if draw
// else FALSE
int IsGameEnded()
{
	int i;
	int j;
	
	for ( i = 0; i < ROW_NUMBER; i++)
	{
		for (j = 0; j < COL_NUMBER-3; j++)
		{
			if (board_server[i][j] != 0 && board_server[i][j] == board_server[i][j + 1] && board_server[i][j] == board_server[i][j + 2] && board_server[i][j] == board_server[i][j + 3])
			{
				return TRUE;
			}
		}
	}
	for (i = 0; i < ROW_NUMBER-3; i++)
	{
		for (j = 0; j < COL_NUMBER; j++)
		{
			if (board_server[i][j] != 0 && board_server[i][j] == board_server[i+1][j] && board_server[i][j] == board_server[i + 2][j] && board_server[i][j] == board_server[i + 3][j] )
			{
				return TRUE;
			}
		}
	}
	for (i = 0; i < ROW_NUMBER-3; i++)
	{
		for (j = 0; j < COL_NUMBER-3; j++)
		{
			if (board_server[i][j] != 0 &&  board_server[i][j] == board_server[i + 1][j + 1] && board_server[i][j] == board_server[i + 2][j + 2] && board_server[i][j] == board_server[i + 3][j + 3] )
			{
				return TRUE;
			}
		}
	}
	for (i = 3; i < ROW_NUMBER; i++)
	{
		for (j = 0; j < COL_NUMBER-3; j++)
		{
			if (board_server[i][j] != 0 &&  board_server[i][j] == board_server[i - 1][j + 1] && board_server[i][j] == board_server[i - 2][j + 2] && board_server[i][j] == board_server[i - 3][j + 3] )
			{
				return TRUE;
			}
		}
	}

	// Detect draw
	
	for (j = 0; j < COL_NUMBER; j++)
	{
		if (board_server[ROW_NUMBER-1][j] == 0)
		{
			return FALSE;
		}
	}
	return DRAW_DETECTED;
}

// creates GAME_ENDED message
void CreateGameEndedMessage(char* SendStr)
{
	strcpy(SendStr, "GAME_ENDED:");
	if (winner != DRAW_DETECTED)
	{
		strcat(SendStr, players_username_arr[winner - 1]);
	}
	else
	{
		strcat(SendStr, "DRAW ");
	}
}

// create RECEIVE_MESSAGE 
void CreateReceiveMessage(char* AcceptedStr, char* SendStr,int Ind)
{
	strcpy(SendStr, "RECEIVE_MESSAGE:");
	strcat(SendStr, players_username_arr[Ind]);
	strcat(SendStr,";");
	strcat(SendStr, &(AcceptedStr[13]));
}

// Initiallizations in the beggining of MainServer
void InitiallizeMainServer()
{
	ThreadInputs[0] = NULL;
	ThreadInputs[1] = NULL;
	ThreadHandles[0] = NULL;
	ThreadHandles[1] = NULL;
	//allocates memory for players_username_arr
	if ((players_username_arr[0] = (char*)malloc(sizeof(MAX_USERNAME_LENGTH))) == NULL)
	{
		fprintf(p_log_file, "Custom message: Error in memory allocation\n");
		printf("Custom message: Error in memory allocation\n");
		exit(1);
	}
	if ((players_username_arr[1] = (char*)malloc(sizeof(MAX_USERNAME_LENGTH))) == NULL)
	{
		fprintf(p_log_file, "Custom message: Error in memory allocation\n");
		printf("Custom message: Error in memory allocation\n");
		exit(1);
	}
	//initialize players_usernames to "' 
	strcpy(players_username_arr[0], "");
	strcpy(players_username_arr[1], "");

	game_started_server = 0;
	notify_game_started = 0;
	turn_player_num = 0;
	game_ended = FALSE;
	winner = 0;

	// Initialize moves array
	InitializePlayMovesArr();
	InitializeBoardServer();

}

//printing to file and screen and closing socket 
void PrintServiceSocketErrorAndCloseSocket(int Ind)
{
	printf("Service socket error while writing, closing thread.\n");
	fprintf(p_log_file, "Service socket error while writing, closing thread.\n");
	closesocket(ThreadInputs[Ind]);
}
