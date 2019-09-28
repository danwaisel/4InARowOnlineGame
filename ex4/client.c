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
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NUM_OF_PLAYERS 2
#ifndef CLIENT_H
#define CLIENT_H

#include <windows.h>
#endif // !CLIENT_H
// Variables -------------------------------------------------------------------
SOCKET m_socket;
//char buffer_new_user_request_message[MAX_MESSAGE_LEN];
//char buffer_play_request_message[MAX_PLAY_REQUEST_MESSAGE_LEN];
char buffer_send_message[MAX_MESSAGE_LEN];
char buffer_recv[MAX_MESSAGE_LEN];
int exit_flag;
int my_turn;
FILE* fp_input_file;
message* p_head_message_buffer;
HANDLE* mutex_for_message_linked_list;
HANDLE hThread[NUM_OF_PLAYERS];
char* AcceptedStrClient;
// Function Declarations -------------------------------------------------------
void PrintBoard(int board[][COL_NUMBER]);
void PrintTurnSwitchMessage(char* AcceptedStr);
int isPlay(char* game_message);
void RecognizeGameMessageType(char* game_message);
void PrintWinnerMessage(char* AcceptedStr);
void CreateSendMessageAndAddToLinkedList(char* game_message);
int isMessage(char* game_message);
void PrintReceivedMessage(char* AcceptedStr);
void PrintErrorMessage(char* AcceptedStr);
int PlayedIllegalMove(char* AcceptedStr);
int isMyTurn(char* AcceptedStr);
void RemoveBackSlashN(char* username);
void AddMessageToLinkedList(message** pp_head_message_buffer, char* buffer_final_message);
void DeleteFirstMessageFromLinkedList();
void CreatePlayMessageAndAddToLinkedList(char* col_index);
void FinishAndCloseAllNoErrors();
void freeLinkedList();
int isExit(char* game_message);
void CreateNewUserRequestMessageAndAddToLinkedList(char* username);


// Function Definitions -------------------------------------------------------

/*
functionality: i.Checks in a loop  for server messages & prints them
ii.	If TRNS_FAILED or TRNS_DISCONNECTED – closing program resources.
iii.Else getting message type- and responds according to it. only if NEW_USER_DECLINED or NEW_USER_ACCEPTED - updates buffer so UI acess the msg and responds.

*/

//Reading data coming from the server, updating buffer_recv to update UI if theres a new msg.
static DWORD RecvDataThread(void)
{
	TransferResult_t RecvRes;
	char message_type[MAX_MESSAGE_TYPE_LEN];
	*buffer_recv = '\0';
	while (1)
	{
		//this condition makes sure we stop getting messages after getting USER_DECLINED msg
		if (*buffer_recv == '\0')
		{
			//checks in a loop  for server messages & prints them (AcceptedStr)
			AcceptedStrClient = NULL;
			//receiving msg
			RecvRes = ReceiveString(&AcceptedStrClient, m_socket);

			if (RecvRes == TRNS_FAILED)
			{
				printf("Server disconnected. Exiting.\n");
				fprintf(p_log_file,"Server disconnected. Exiting.\n");
				FinishAndCloseAllNoErrors();
				exit(0);
			}
			else if (RecvRes == TRNS_DISCONNECTED)
			{
				printf("Server disconnected. Exiting.\n");
				fprintf(p_log_file,"Server disconnected. Exiting.\n");
				FinishAndCloseAllNoErrors();
				exit(0);
			}
			else
			{
				fprintf(p_log_file, "Received from server: %s\n", AcceptedStrClient);
			}
			//getting message type
			GetMessageType(AcceptedStrClient, message_type);
			//if BOARD_VIEW- -prints board view
			if (strcmp(message_type, "BOARD_VIEW") == 0)
			{
				PrintBoardView(AcceptedStrClient);
			}
			//if TURN_SWITCH- print turn switch msg and update my_turn
			else if (strcmp(message_type, "TURN_SWITCH") == 0)
			{
				PrintTurnSwitchMessage(AcceptedStrClient);
				//update my_turn only if its this username's turn (both usernames get the TURN_SWITCH message- the name in the message body determines whose turn is now
				if (isMyTurn(AcceptedStrClient) == TRUE)
				{
					my_turn = TRUE;
				}
			}
			//if GAME_ENDED- print who won
			else if (strcmp(message_type, "GAME_ENDED") == 0)
			{
				PrintWinnerMessage(AcceptedStrClient);
			}
			else if (strcmp(message_type, "NEW_USER_DECLINED") == 0)
			{
				printf("Request to join was refused\n");
				FinishAndCloseAllNoErrors();
				exit(0);
			}
			else if (strcmp(message_type, "NEW_USER_ACCEPTED") == 0)
			{
				strcpy(buffer_recv, "NEW_USER_ACCEPTED");
			}
			else if (strcmp(message_type, "RECEIVE_MESSAGE") == 0)
			{
				PrintReceivedMessage(AcceptedStrClient);
			}
			else if (strcmp(message_type, "PLAY_ACCEPTED") == 0)
			{
				printf("Well played\n");
			}
			//if you played not in your turn or played illegal move (in your turn) or game not started
			else if (strcmp(message_type, "PLAY_DECLINED") == 0)
			{

				PrintErrorMessage(AcceptedStrClient);
				//if played illegal move in your turn- giving the turn back to you.(didnt really play the last turn)
				if (PlayedIllegalMove(AcceptedStrClient) == TRUE)
				{
					my_turn = TRUE;
				}
			}
			//if msg type: GAME_STARTED 
			else if (strcmp(message_type, "GAME_STARTED") == 0)
			{
				printf("Game is on!\n");
			}
			

			free(AcceptedStrClient);
		}
		
	}

	return 0;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

/*
functionality: Sending data to the server:
1)Polling the linked list mutex- and sends play/chat message  from it to server
2)Checking if client UI wants to send NEW_USER_REQUEST and sends it to server (before game started) 
*/
static DWORD SendDataThread(void)
{
	DWORD wait_code;
	int ret_val;
	TransferResult_t SendRes;
	while (1)
	{
		wait_code = WaitForSingleObject(mutex_for_message_linked_list, 0);
		if (wait_code == WAIT_FAILED)
		{
			printf("Custom message: Error in WaitForSingleObject when polling anchor mutex\n");
			fprintf(p_log_file, "Custom message: Error in WaitForSingleObject when polling anchor mutex\n");
			exit(1);
		}

		/*
		*-----------------------------------*
		*          Critical Section         *
		*-----------------------------------*
		*/

		if (wait_code == WAIT_OBJECT_0)
		{
			//waits until gets message from the user- buffer_send_message != \0
			if (p_head_message_buffer != NULL)
			{
				//sends SEND_MESSAGE to the server
				//update buffer_send_message to the first message in the LL, delete it from the LL and send it to server.
				strcpy(buffer_send_message, p_head_message_buffer->message_body);
				DeleteFirstMessageFromLinkedList();
			}

			/*
			*-----------------------------------*
			*            Release Mutex          *
			*-----------------------------------*
			*Sharing is caring        *
			*/

			ret_val = ReleaseMutex(mutex_for_message_linked_list);
			if (FALSE == ret_val)
			{
				printf("Custom message: Error when releasing anchor mutex \n");
				fprintf(p_log_file, "Custom message: Error when releasing anchor mutex \n");
				exit(1);
			}
		}
		//sending the play/message message that was taken out of the LL
		if (*buffer_send_message != '\0')
		{
			fprintf(p_log_file, "Sent to server: %s\n", buffer_send_message);
			SendRes = SendString(buffer_send_message, m_socket);
			if (SendRes == TRNS_FAILED)
			{
				printf("send() failed, error %d\n", WSAGetLastError());
				fprintf(p_log_file, "send() failed, error %d\n", WSAGetLastError());
				exit(1);
			}
			*buffer_send_message = '\0';
		}
		/*
		//waits until gets NEW_USER_REQUEST message- bufer_new_user_request != \0
		if (*buffer_new_user_request_message != '\0')
		{
			//---step 4---sends NEW_USER_REQUEST to the server
			fprintf(p_log_file, "Sent to server: %s\n", buffer_new_user_request_message);
			SendRes = SendString(buffer_new_user_request_message, m_socket);
			if (SendRes == TRNS_FAILED)
			{
				fprintf(p_log_file, "send() failed, error %d\n", WSAGetLastError());
				printf("send() failed, error %d\n", WSAGetLastError());
				return 0x555;
			}
			*buffer_new_user_request_message = '\0';
		}*/
	}
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

/*
functionality: Sending data to the server:
1)connection to server, opening mutex and Send&Recv threads,
2)waiting for username
3)infinite loop:
	first part-   if the user can send commands- Recognizing message type- PLAY_REQUEST /SEND_MESSAGE_REQUEST /exit /Illegal command- and pass it through buffer/LL to Send thread.
	second part-  user can't send commands- - waiting till user is accepted or refused- getting from server through Recv thread(NEW_USER_ACCEPTED or NEW_USER_DECLINED)

*/
void MainClient()
{
	SOCKADDR_IN clientService;
	char FailConnectionMessage[MAX_CONNECTION_ERROR_NUM];
	char ConnectionMessage[MAX_CONNECTION_ERROR_NUM];
	int isGettingUserDemands;
	char game_message[MAX_GAME_MESSAGE_LEN];
	char message_type[MAX_MESSAGE_TYPE_LEN];
	*buffer_recv = '\0';
	//*buffer_play_request_message = '\0';
	//*buffer_new_user_request_message = '\0';
	exit_flag = FALSE;
	my_turn = FALSE;
	fp_input_file = NULL;
	isGettingUserDemands = FALSE;
	p_head_message_buffer = NULL;
	*buffer_send_message = '\0';
	*buffer_recv = '\0';
	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
					 //The WSADATA structure contains information about the Windows Sockets implementation.

					 //Call WSAStartup and check for errors.
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		fprintf(p_log_file, "Custom message: Error at WSAStartup()\n");
		printf("Custom message: Error at WSAStartup()\n");
		exit(1);
	}
	//if file mode-opening input file
	if (is_human_mode == FALSE)
	{
		fp_input_file = fopen(input_file, "r");
		if (fp_input_file == NULL)
		{
			printf("Error opening input file\n");
			exit(1);
		}
	}
	//creating mutex for the linked list-  UI adds play/chat messages and Send thread consumes- takes a message from the LL .
	mutex_for_message_linked_list = CreateMutex(
		NULL,	/* default security attributes */
		FALSE,	/* initially not owned */
		NULL);	/* unnamed mutex */
	if (mutex_for_message_linked_list == NULL)
	{
		fprintf(p_log_file, "Custom message: Error when creating mutex: %d\n", GetLastError());
		printf("Custom message: Error when creating mutex: %d\n", GetLastError());
		exit(1);
	}

	//Call the socket function and return its value to the m_socket variable. 
	// For this application, use the Internet address family, streaming sockets, and the TCP/IP protocol.

	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {
		fprintf(p_log_file, "Custom message: Error at socket(): %ld\n", WSAGetLastError());
		printf("Custom message: Error at socket(): %ld\n", WSAGetLastError());
	
		WSACleanup();
		exit(1);
	}
	/*
	The parameters passed to the socket function can be changed for different implementations.
	Error detection is a key part of successful networking code.
	If the socket call fails, it returns INVALID_SOCKET.
	The if statement in the previous code is used to catch any errors that may have occurred while creating
	the socket. WSAGetLastError returns an error number associated with the last error that occurred.
	*/
	//For a client to communicate on a network, it must connect to a server.
	// Connect to a server.
	//Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	clientService.sin_port = htons(server_port); //Setting the port to connect to.

												 /*
												 AF_INET is the Internet address family.
												 */

												 // Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
												 // Check for general errors.
	//------step3---- if connection failed-print Fail Connection error
	if (connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR)
	{
		CreateFailConnectionMessage(FailConnectionMessage);
		fprintf(p_log_file, "%s", FailConnectionMessage);
		printf("%s", FailConnectionMessage);
		WSACleanup();
		exit(1);

	}
	//------step2----- print connected message
	CreateConnectionMessage(ConnectionMessage);
	fprintf(p_log_file, "%s", ConnectionMessage);
	printf("%s", ConnectionMessage);


	// Send and receive data.
	/*
	In this code, two integers are used to keep track of the number of bytes that are sent and received.
	The send and recv functions both return an integer value of the number of bytes sent or received,
	respectively, or an error. Each function also takes the same parameters:
	the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.

	*/

	hThread[0] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)SendDataThread,
		NULL,
		0,
		NULL
	);
	if (hThread[0] == NULL)
	{
		printf("Custom message: Error in opening thread\n");
		fprintf(p_log_file, "Custom message: Error in opening thread\n");
		exit(1);
	}
	hThread[1] = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)RecvDataThread,
		NULL,
		0,
		NULL
	);
	if (hThread[1] == NULL)
	{
		printf("Custom message: Error in opening thread\n");
		fprintf(p_log_file, "Custom message: Error in opening thread\n");
		exit(1);
	}

	//------step4---
	printf("Please enter username\n");
	//get user name from human
	if (is_human_mode==TRUE)
	{
		gets(username);
	}
	//gets username fron file
	else
	{
		fgets(username, MAX_USERNAME_LENGTH, fp_input_file);
		RemoveBackSlashN(username);
	}
	
	//create NEW_USER_REQUEST message and saves it in buffer
	CreateNewUserRequestMessageAndAddToLinkedList(username);

	/*infinite loop: 
	first part-   if the user can send commands- after getting a valid user name
	second part-  user can't send commands- before getting a valid user name- waiting till user is accepted or refused. (client checks if he got msg from the server. if so- respond according to msg type. )
	
	*/
	while (TRUE)
	{
		// if user is after the stage of valid username 
		if (isGettingUserDemands == TRUE)
		{
			// if file mode- only if it's players turn- waits for game message from the player, recognize and send it to server 
			if (is_human_mode == FALSE)
			{
				if (my_turn == TRUE)
				{
					fgets(game_message, MAX_USERNAME_LENGTH, fp_input_file);
					RemoveBackSlashN(game_message);
					RecognizeGameMessageType(game_message);
					if (exit_flag == TRUE)
					{
						goto Exit_client;
					}
				}
			}
			// if human mode- waits for game message from the player, recognize and send it to server 
			else
			{
				gets(game_message);
				RecognizeGameMessageType(game_message);
				if (exit_flag == TRUE)
				{
					goto Exit_client;
				}
			}
		}

		//checks if user still in the stage before entring a valid username and buffer is full- client got a msg from server (throught thread recv)
		else if (isGettingUserDemands == FALSE && *buffer_recv != '\0')
		{
			//if msg type: NEW_USER_ACCEPTED - empty buffer
			GetMessageType(buffer_recv,message_type);
			if (strcmp(message_type, "NEW_USER_ACCEPTED") == 0)
			{
				*buffer_recv = '\0';
				isGettingUserDemands = TRUE;
			}
		}
	}

Exit_client:
	freeLinkedList();
	fclose(p_log_file);
	if (is_human_mode == FALSE)
	{
		fclose(fp_input_file);
	}

	//closing only send recv threads
	TerminateThread(hThread[0], 0x555);
	TerminateThread(hThread[1], 0x555);


	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	
	free(AcceptedStrClient);
	

	closesocket(m_socket);

	CloseHandle(mutex_for_message_linked_list);

	WSACleanup();
	exit(0);
}



/*
[IN]: global server_port
[OUT]: -
return value:SUCCESS_CODE if succeeded, otherwise ERROR_CODE
functionality: preparing the faliure connection message
*/


void CreateFailConnectionMessage(char* p_FailConnectionMessage)
{
	strcpy(p_FailConnectionMessage, "Failed connecting to server on 127.0.0.1:");
	strcat(p_FailConnectionMessage, server_port);
	strcat(p_FailConnectionMessage, "\n");

}

/*
[IN]: global server_port
[OUT]: -
return value:SUCCESS_CODE if succeeded, otherwise ERROR_CODE
functionality: preparing the faliure connection message
*/


void CreateConnectionMessage(char* p_ConnectionMessage)
{
	strcpy(p_ConnectionMessage, "Connected to server on 127.0.0.1:");
	strcat(p_ConnectionMessage, server_port);
	strcat(p_ConnectionMessage, "\n");

}

/*
[IN]: string-a message from client
[OUT]: string -message_type
return value:SUCCESS_CODE if succeeded, otherwise ERROR_CODE
functionality: gets the whole message in format <message_type> or <message_type>:<param_list>\0 and extract the <message_type, saves it in output param message_type
*/

void GetMessageType(char* SendStr,char* message_type)
{
	int i = 0;
	while (SendStr[i] != ':' && SendStr[i] != '\0')
	{
		message_type[i] = SendStr[i];
		i ++ ;
	}
	message_type[i] = '\0';
}
/*todo- delete inrelavent
[IN]: string-username
[OUT]: string -buffer_new_user_request_message
return value:-
functionality: gets username and creates the whole message NEW_USER_REQUEST:username\0. saves it in output param buffer_new_user_request_message.
*/
void CreateNewUserRequestMessage(char* username, char* buffer_new_user_request_message)
{
	strcpy(buffer_new_user_request_message, "NEW_USER_REQUEST:");
	strcat(buffer_new_user_request_message, username);

}


/*
[IN]: string
[OUT]: 
return value:-
functionality: gets board view message and print the board accordingly
*/
void PrintBoardView(char* accepted_str)
{
	int board[ROW_NUMBER][COL_NUMBER] = { 0 };
	int column_height[COL_NUMBER] = { 0 };
	int i=11;
	if (accepted_str[i - 1] == '\0')
	{
		PrintBoard(board);
		return;
	}
	int move;
	//accepted_str is in format: BOARD_VIEW:1;2;3;0...4\0 so index 11 is the first move, we skip the ";" 
	while(accepted_str[i] != '\0')
	{
		if (accepted_str[i] == ';')
		{
			i++;
			continue;
		}
		move = accepted_str[i] - '0';
		if((i/2)%2!=0)
		{
			board[column_height[move]][move] = RED_PLAYER;
			column_height[move] = column_height[move] + 1;
		}
		else
		{
			board[column_height[move]][move] = YELLOW_PLAYER;
			column_height[move] = column_height[move] + 1;
		}
		i++;
	}
	PrintBoard(board);
}
/*
[IN]: board 2d array
[OUT]: 
return value:-
functionality: gets the board 2d array and print it with colors and all
*/
void PrintBoard(int board[][COL_NUMBER])
{
	//This handle allows us to change the console's color
	HANDLE  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	int row, column;

	//Draw the board
	for (row = ROW_NUMBER-1; row >=0; row--)
	{
		for (column = 0; column < COL_NUMBER; column++)
		{
			printf("| ");
			if (board[row][column] == RED_PLAYER)
				SetConsoleTextAttribute(hConsole, RED);

			else if (board[row][column] == YELLOW_PLAYER)
				SetConsoleTextAttribute(hConsole, YELLOW);

			printf("O");

			SetConsoleTextAttribute(hConsole, BLACK);
			printf(" ");
		}
		printf("\n");

		//Draw dividing line between the rows
		for (column = 0; column < COL_NUMBER; column++)
		{
			printf("----");
		}
		printf("\n");
	}

	//free the handle
	//CloseHandle(hConsole);

}
/*
[IN]: string message
[OUT]: 
return value:-
functionality: prints TURN_SWITCH message
*/
void PrintTurnSwitchMessage(char* AcceptedStr) 
{
	fprintf(p_log_file, "%s's turn\n", &(AcceptedStr[12]));
	printf("%s's turn\n", &(AcceptedStr[12]));
}

/*
[IN]: string game message, typed by the user during thhe game
[OUT]: buffers
return value:-
functionality:calls Create... functions that put messages to be sent from UI so send thread in buffer
*/
void RecognizeGameMessageType(char* game_message)
{
	if (isPlay(game_message)==TRUE)
	{
		//if user play his turn- update my_turn to switch turns
		my_turn = FALSE;
		CreatePlayMessageAndAddToLinkedList(&game_message[5]);
		//CreatePlayRequestMessage(game_message[5]);
	}
	else if (isMessage(game_message) == TRUE)
	{
		CreateSendMessageAndAddToLinkedList(game_message);
	}
	else if (isExit(game_message) == TRUE)
	{
		exit_flag = TRUE;
	}
	else
	{
		printf("Error: Illegal command\n");
		fprintf(p_log_file,"Error: Illegal command\n");
	}
}
/*
[IN]: string game message, typed by the user during thhe game 
[OUT]:
return value: TRUE if the msg is in format: play <col_num>, otherwise FALSE
functionality:checks if the msg is in format: play <col_num>
*/
int isPlay(char* game_message)
{
	
	if ((game_message[0] == 'p') && (game_message[1] == 'l') && (game_message[2] == 'a') && (game_message[3] == 'y') && (game_message[4] == ' '))
	{
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
	
	
}

/*
[IN]: string message
[OUT]:
return value:-
functionality: prints GAME ENDED message
*/
void PrintWinnerMessage(char* AcceptedStr)
{
	if (AcceptedStr[11] == 'D' && AcceptedStr[12] == 'R' && AcceptedStr[13] == 'A' && AcceptedStr[14] == 'W' && AcceptedStr[15] == ' ' && AcceptedStr[16] == '\0')
	{
		printf("Game ended. Everybody wins!\n");
		fprintf(p_log_file,"Game ended. Everybody wins!\n");
	}
	else 
	{
		printf("Game ended. The winner is %s!\n", &AcceptedStr[11]);
		fprintf(p_log_file, "Game ended. The winner is %s!\n", &AcceptedStr[11]);
	}
	
}

/*
[IN]: string game message, typed by the user during thhe game
[OUT]:
return value: TRUE if the msg is in format: message <messasge body>, otherwise FALSE
functionality:checks if the msg is in format: message <messasge body>
*/

int isMessage(char* game_message)
{
	if (strlen(game_message) >= 8)
	{
		if ((game_message[0] == 'm') && (game_message[1] == 'e') && (game_message[2] == 's') && (game_message[3] == 's') && (game_message[4] == 'a') && (game_message[5] == 'g') && (game_message[6] == 'e') && (game_message[7] == ' '))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

}

/*
[IN]: string message_body- body of the message user types
[OUT]:-
return value:-
functionality: Creates the SEND_MESSAGE message to be transfered from UI to send thread,adds it to the end of the messages LL.
*/
void CreateSendMessageAndAddToLinkedList(char* game_message)
{
	DWORD wait_code;
	int ret_val;
	char buffer_final_message[MAX_MESSAGE_LEN];
	char game_message_to_send[MAX_MESSAGE_LEN];
	int i = 8;
	int j = 0;
	strcpy(buffer_final_message, "SEND_MESSAGE:");
	while (game_message[i] != '\0')
	{
		if (game_message[i] != ' ')
		{
			game_message_to_send[j] = game_message[i];
			i++;
			j++;
		}
		else
		{
			game_message_to_send[j] = ';';
			j++;
			game_message_to_send[j] = ' ';
			j++;
			game_message_to_send[j] = ';';
			j++;
			i++;
		}
	}
	game_message_to_send[j] = '\n';
	game_message_to_send[j + 1] = '\0';
	strcat(buffer_final_message, game_message_to_send);
	
	wait_code = WaitForSingleObject(mutex_for_message_linked_list, INFINITE);
	if (wait_code == WAIT_FAILED)
	{
		printf("Error in WaitForSingleObject  when polling anchor mutex\n");
		fprintf(p_log_file, "Error in WaitForSingleObject when polling anchor mutex\n");
		exit(1);
	}

	/*
	*-----------------------------------*
	*          Critical Section         *
	*-----------------------------------*
	*/

	if (wait_code == WAIT_OBJECT_0)
	{
		AddMessageToLinkedList(&p_head_message_buffer, buffer_final_message);
		/*
		*-----------------------------------*
		*            Release Mutex          *
		*-----------------------------------*
		*Sharing is caring        *
		*/

		ret_val = ReleaseMutex(mutex_for_message_linked_list);
		if (FALSE == ret_val)
		{
			printf("Error when releasing anchor mutex \n");
			fprintf(p_log_file, "Error when releasing anchor mutex \n");
			exit(1);
		}
	}
}
/*
[IN]: string message_body- body of the message user types
[OUT]:-
return value:-
functionality: Creates the PLAY_REQUEST message to be transfered from UI to send thread, adds it to the end of the messages LL.
*/
void CreatePlayMessageAndAddToLinkedList(char* col_index)
{
	DWORD wait_code;
	int ret_val;
	char buffer_final_message[MAX_MESSAGE_LEN];

	int i = 8;
	int j = 0;
	strcpy(buffer_final_message, "PLAY_REQUEST:");
	strcat(buffer_final_message, col_index);
	//buffer_final_message[13] = col_index;
	//buffer_final_message[14] = '\0';
	wait_code = WaitForSingleObject(mutex_for_message_linked_list, INFINITE);
	if (wait_code == WAIT_FAILED)
	{
		printf("Error in WaitForSingleObject  when polling anchor mutex\n");
		fprintf(p_log_file, "Error in WaitForSingleObject when polling anchor mutex\n");
		exit(1);
	}

	/*
	*-----------------------------------*
	*          Critical Section         *
	*-----------------------------------*
	*/

	if (wait_code == WAIT_OBJECT_0)
	{
		AddMessageToLinkedList(&p_head_message_buffer, buffer_final_message);
		/*
		*-----------------------------------*
		*            Release Mutex          *
		*-----------------------------------*
		*Sharing is caring        *
		*/

		ret_val = ReleaseMutex(mutex_for_message_linked_list);
		if (FALSE == ret_val)
		{
			printf("Error when releasing anchor mutex \n");
			fprintf(p_log_file, "Error when releasing anchor mutex \n");
			exit(1);
		}
	}
}
/*
[IN]: string message_body- body of the message user types
[OUT]:-
return value:-
functionality: Creates the NEW_USER_REQUEST message to be transfered from UI to send thread, adds it to the end of the messages LL.
*/
void CreateNewUserRequestMessageAndAddToLinkedList(char* username)
{
	DWORD wait_code;
	int ret_val;
	char buffer_final_message[MAX_MESSAGE_LEN];

	int i = 8;
	int j = 0;
	strcpy(buffer_final_message, "NEW_USER_REQUEST:");
	strcat(buffer_final_message, username);
	wait_code = WaitForSingleObject(mutex_for_message_linked_list, INFINITE);
	if (wait_code == WAIT_FAILED)
	{
		printf("Error in WaitForSingleObject when polling anchor mutex\n");
		fprintf(p_log_file,"Error in WaitForSingleObject when polling anchor mutex\n");
		exit(1);
	}

	/*
	*-----------------------------------*
	*          Critical Section         *
	*-----------------------------------*
	*/

	if (wait_code == WAIT_OBJECT_0)
	{
		AddMessageToLinkedList(&p_head_message_buffer, buffer_final_message);
		/*
		*-----------------------------------*
		*            Release Mutex          *
		*-----------------------------------*
		*Sharing is caring        *
		*/

		ret_val = ReleaseMutex(mutex_for_message_linked_list);
		if (FALSE == ret_val)
		{
			printf("Error when releasing anchor mutex \n");
			fprintf(p_log_file,"Error when releasing anchor mutex \n");
			exit(1);
		}
	}
}

/*
[IN]: string game message, typed by the user during thhe game
[OUT]:
return value: TRUE if the msg is in format: exit, otherwise FALSE
functionality:checks if the msg is in format: exit
*/

int isExit(char* game_message)
{
	if (strlen(game_message)== 4)
	{
		if ((game_message[0] == 'e') && (game_message[1] == 'x') && (game_message[2] == 'i') && (game_message[3] == 't') )
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

}

/*
[IN]: string chat message, typed by the user during the game
[OUT]:-
return value: -
functionality:prints the message that the client got. (gets in format RECEIVE_MESSAGE:<param>;<param>;<param>\n\0 and prints the chat message)
*/

void PrintReceivedMessage(char* AcceptedStr)
{
	int i = 16;
	int j = 0;
	char message_for_print[MAX_MESSAGE_LEN];


	
	while (AcceptedStr[i] != ';')
	{
		message_for_print[j] = AcceptedStr[i];
		i++;
		j++;
	}
	message_for_print[j]=':';
	j++;
	message_for_print[j] = ' ';
	j++;
	i++;
	while (AcceptedStr[i] != '\0')
	{
		if (AcceptedStr[i] == ';')
		{
			i++;
			continue;
		}
		message_for_print[j] = AcceptedStr[i];
		i++;
		j++;
	}
	message_for_print[j] = '\0';
	printf("%s", message_for_print);
	
}

/*
[IN]: string -error message- in format PLAY_DECLINED:<error message>\0
[OUT]:-
return value: -
functionality:prints the error message that the client got. 
*/

void PrintErrorMessage(char* AcceptedStr)
{
	int i = 14;
	int j = 0;
	char message_for_print[MAX_MESSAGE_LEN];



	while (AcceptedStr[i] != '\0')
	{
		message_for_print[j] = AcceptedStr[i];
		i++;
		j++;
	}
	message_for_print[j] = '\0';
	printf("Error: %s\n", message_for_print);

}


/*
[IN]: string -error message- in format PLAY_DECLINED:<error message>\0
[OUT]: -
return value: TRUE if <error message> == Illegal move, else FALSE
functionality: gets a message and returns TRUE if <error message> == Illegal move 
*/
int PlayedIllegalMove(char* AcceptedStr)
{
	if (strcmp(AcceptedStr, "PLAY_DECLINED:Illegal move")==0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	
}

/*
[IN]: string -in format TURN_SWITCH:<username>\0
[OUT]: -
return value: TRUE if username's turn, else FALSE
functionality: gets a message and returns TRUE if the player's username is written in the message.
*/
int isMyTurn(char* AcceptedStr)
{
	if (strcmp(username, &(AcceptedStr[12]))==0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
/*
[IN]: string 
[OUT]: string
return value: -
functionality: gets a string and removes the \n in the end if exits.
*/
void RemoveBackSlashN(char* string_line)
{
	int i = 0;
	while (string_line[i]!='\n' && string_line[i] != '\0')
	{
		i++;
	}
	string_line[i] = '\0';
}

/*
[IN]: string-buffer_final_message
[OUT]: global pp_head_message_buffer 
return value: SUCCESS_CODE if succceed, else ERROR_CODE
functionality: creates a new message struct from a string (message body) and adds it to the end of the linked list.
*/
void AddMessageToLinkedList(message** pp_head_message_buffer, char* buffer_final_message) 
{
	message* p_iter = *pp_head_message_buffer;
	message* p_new_message = (message*)malloc(sizeof(message));
	if (p_new_message == NULL)
	{
		fprintf(p_log_file, "Custom message: Error in memory allocation\n");
		printf("Custom message: Error in memory allocation\n");
		exit(1);
	}
	strcpy(p_new_message->message_body, buffer_final_message);
	p_new_message->next_message = NULL;
	
	if (*pp_head_message_buffer == NULL)
	{
		*pp_head_message_buffer = p_new_message;
		return;
	}
	while (p_iter->next_message != NULL)
	{
		p_iter = p_iter->next_message;
	}
	p_iter->next_message = p_new_message;
	return;
}

/*
[IN]: 
[OUT]: global head of the message linked list
return value: -
functionality:deletes the head of the linked list and frees it.
*/
void DeleteFirstMessageFromLinkedList()
{
	message* p_message_to_delete = p_head_message_buffer;
	p_head_message_buffer = p_head_message_buffer->next_message;
	free(p_message_to_delete);
}

/*
[IN]:
[OUT]: 
return value: -
functionality:Free all resources, free memory allocation and close all handles and sockets.
*/
void FinishAndCloseAllNoErrors()
{
	closesocket(m_socket);
	freeLinkedList();
	fclose(p_log_file);
	if (is_human_mode == FALSE)
	{
		fclose(fp_input_file);
	}

	//closing only send thread- recv thread will be terminated by exit
	TerminateThread(hThread[0], 0x555);
	free(AcceptedStrClient);

	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

	
	CloseHandle(mutex_for_message_linked_list);

	WSACleanup();

	
}
/*
[IN]:-
[OUT]: global head of the message linked list
return value: -
functionality:cleans Linked List and free memory
*/

void freeLinkedList()
{
	message* p_message_iter = p_head_message_buffer;
	while (p_head_message_buffer != NULL)
	{
		p_message_iter = p_head_message_buffer;
		p_head_message_buffer = p_head_message_buffer->next_message;
		free(p_message_iter);
	}

}
