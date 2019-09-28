/*
Authors: Dan Waisel 312490808 Yuval Tal 205704612
Project: ex4
Description: Online game of Connect Four. This project include Client and Server.
*/

// Includes --------------------------------------------------------------------

#include "server.h"
#include "client.h"
#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "send_receive.h"
#include <stdbool.h>
#include <windows.h>
#include <stdlib.h>
#include <limits.h>
#include <tchar.h>
#include <strsafe.h>

// Constants -------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS
#define ERROR_CODE ((int)(-1))
#define SUCCESS_CODE ((int)(0))
#define MAX_MODE_LENGTH 10
#define MAX_LOG_PATH_LENGTH 1000


// Variables -------------------------------------------------------------------


// Function Declarations -------------------------------------------------------


// Function Definitions -------------------------------------------------------

int main(int argc, char *argv[])
{
	server_port_int = atoi(argv[3]);
	char mode[MAX_MODE_LENGTH];
	char log_path[MAX_LOG_PATH_LENGTH];

	
	p_log_file = NULL;
	strcpy(server_port, argv[3]);
	strcpy(mode, argv[1]);
	strcpy(log_path, argv[2]);
	p_log_file = fopen(log_path, "w");


	if (p_log_file == NULL)
	{
		printf("Error opening log file\n");
		exit(1);
	}
	if (strcmp(mode, "server")==0)
	{
		printf("Server mode\n");
		MainServer();
	}
	else if (strcmp(mode, "client") == 0)
	{

		//human mode
		if (strcmp(argv[4], "human") == 0)
		{
			is_human_mode = TRUE;
			MainClient();
		}
		//file input mode
		else
		{
			is_human_mode = FALSE;
			strcpy(input_file, argv[5]);
			printf("Client mode\n");
			MainClient();
		}

	}
	else
	{
		printf("wrong mode given");
	}
	
}