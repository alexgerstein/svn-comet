#ifndef CORE_H
#define CORE_H

#include <chipmunk/chipmunk.h>
#include "physics.h"

enum CommandType { ADD_BODY, UPDATE_BODIES, REMOVE_BODY, SLICE, END, LOAD };


typedef struct  
{
  char *to_send;
  int space_index;
} SendData;

// An I/O function that parses the .lvl file to create levels. Objects are listed with their locations.
char* core_load_level(GameState *game_state, int level_number);

int core_client_handle_command (cpSpace *space, char *command);

int core_server_handle_slice (cpSpace *space, char *command);

void core_generate_slice(char *command, double x1, double y1, double x2, double y2);
void core_generate_end_level(char *command);

void load_test_level (cpSpace *space);
void core_iterate_protocol(GameState *game_state, char *command);
void core_server_generate_updates (char *command, cpSpace *space, int connfd);
#endif
