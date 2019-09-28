/*
 Authors: Dan Waisel 312490808 Yuval Tal 205704612
 Project: ex4
 Description: Online game of Connect Four. This project include Client and Server.
 */
#pragma once


/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

void MainServer();
void CreateNewUserDeclinedMessage(char* SendStr);
void CreateTurnSwitchMessage(char* SendStr, int turn_player_num);
void InitializeBoardServer();
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


