/*
 Authors: Dan Waisel 312490808 Yuval Tal 205704612
 Project: ex4
 Description: Online game of Connect Four. This project include Client and Server.
 */
#pragma once
#ifndef CLIENT_H
#define CLIENT_H
#define MAX_MESSAGE_LEN 400

typedef struct message_s
{
	char message_body[MAX_MESSAGE_LEN];
	struct message_s* next_message;
}message;
void MainClient();

void GetMessageType(char* SendStr, char* message_type);
void CreateConnectionMessage(char* p_ConnectionMessage);
void CreateFailConnectionMessage(char* p_FailConnectionMessage);
void CreateNewUserRequestMessage(char* username, char* buffer_new_user_request_message);
void PrintBoardView(char* accepted_str);
int isPlay(char* game_message);
void RecognizeGameMessageType(char* game_message);
void PrintWinnerMessage(char* AcceptedStr);
void CreateSendMessageAndAddToLinkedList(char* game_message);
int isMessage(char* game_message);
void PrintReceivedMessage(char* AcceptedStr);
int PlayedIllegalMove(char* AcceptedStr);
int isMyTurn(char* AcceptedStr);
void RemoveBackSlashN(char* username);
void AddMessageToLinkedList(message** pp_head_message_buffer, char* buffer_final_message);
void DeleteFirstMessageFromLinkedList();
void CreatePlayMessageAndAddToLinkedList(char* col_index);
void FinishAndCloseAllNoErrors();
void freeLinkedList();
void CreateNewUserRequestMessageAndAddToLinkedList(char* username);
#endif // !CLIENT_H

