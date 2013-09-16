#ifndef CLIENT_H
#define CLIENT_H

#include <chipmunk/chipmunk.h>
#include <gtk/gtk.h>
#include <pthread.h>

#include "physics.h"

typedef struct {
  GameState *game_state;

  int sockfd;
  int send_waiting;
  int recv_waiting;

  int send_next_level;

  char *command;
  //char *command;
  // Array of 4 vertices that store (original_x, original_y) & (curr_x, curr_y)
  double *vertices;
  // Reference to the original lock
  pthread_mutex_t *slice_lock;
  pthread_mutex_t *draw_lock;
  pthread_mutex_t *client_lock;
} ClientData;

ClientData *client_data_new (const char*, const char*, pthread_mutex_t draw_lock, pthread_mutex_t drag_lock);

int client_send (ClientData*);

int client_receive (ClientData*, char *);

int client_update (ClientData*);

int client_broadcast (ClientData*);

int client_handle_communication (ClientData*);

void client_data_destroy (ClientData*);

#endif
