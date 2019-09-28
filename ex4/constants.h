#pragma once

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/


/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#define SERVER_ADDRESS_STR "127.0.0.1"//local host
#define SERVER_PORT 2345
#define MAX_USERNAME_LENGTH 31
#define MAX_ERROR_MESSAGE_LEN 30
#define MAX_INPUT_MODE_LENGTH 10
#define MAX_INPUT_FILE_LENGTH 1000
#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )
#define MAX_CONNECTION_ERROR_NUM 100
#define MAX_MESSAGE_LEN 400
#define MAX_INPUT_MESSAGE_LEN 100
#define MAX_MESSAGE_TYPE_LEN 20
#define MAX_GAME_MESSAGE_LEN 100
#define MAX_PLAY_REQUEST_MESSAGE_LEN 20
#define ROW_NUMBER 6
#define COL_NUMBER 7
#define ROW_NUMBER_X_COL_NUM 42
#define RED_PLAYER 1
#define YELLOW_PLAYER 2
#define RED 204
#define YELLOW 238
#define BLACK 15
int server_port_int;
char server_port[5];
FILE* p_log_file;
char username[MAX_USERNAME_LENGTH]; 
int is_human_mode;
char input_file[MAX_INPUT_FILE_LENGTH];

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
