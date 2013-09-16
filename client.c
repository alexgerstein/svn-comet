/* Client.c */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

#include "client.h"
#include "core.h"


ClientData *client_data_new (const char *ip_addr, const char *server_port, pthread_mutex_t draw_lock, pthread_mutex_t drag_lock) {
  ClientData *cd = (ClientData *) malloc(sizeof(ClientData));
  if ((cd->sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Problem in creating the socket");
    return NULL;
  }

  struct sockaddr_in servaddr;
  memset (&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip_addr);
  servaddr.sin_port = htons(atoi(server_port));

  int x;

  if ((x = connect(cd->sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
    return NULL;
  }

  cd->game_state = NULL;

  // Initialize the vertices and use the address of the mutex stored in memory
  cd->vertices = (double *) malloc (4 * sizeof(double));
  cd->draw_lock = &(draw_lock);
  cd->slice_lock = &(drag_lock);

  cd->recv_waiting = 0;
  cd->send_waiting = 0;

  cd->command = (char *) calloc (200, sizeof(char));
  
  return cd;
}

int client_send (ClientData *cd) {
  if (cd->send_waiting == 1) {
    send (cd->sockfd, cd->command, strlen(cd->command), 0);
    free(cd->command);
    cd->command = (char *)calloc(200, sizeof(char));
    cd->send_waiting = 0;
    //memset(cd->command, " ", sizeof(cd->command)); 
  }
	return EXIT_SUCCESS;
}

int client_receive (ClientData *cd, char *recvline) {
  struct timeval tv = {0, 1000};
  fd_set readfds;
  
  FD_ZERO(&readfds);
  FD_SET(cd->sockfd, &readfds);

  if (select(cd->sockfd + 1, &readfds, NULL, NULL, &tv) < 0) {
    perror("recv");
    return 0;
  }

  if (FD_ISSET(cd->sockfd, &readfds)) {
    int n = recv(cd->sockfd, recvline, 3000, 0);
    if (n != 0) {
      cd->recv_waiting = 1;
    }
  }
  
  return 0;
}

int client_update (ClientData *cd) {
  pthread_mutex_lock (cd->draw_lock);

  char *recvline = (char *) calloc (3000, sizeof(char));
  client_receive (cd, recvline);

  if (cd->recv_waiting) {
    if ( strcmp(recvline, " 0 SLICED \n") == 0) {
      cd->game_state->slices -= 1;
      free (recvline);
    }

    else {
    core_iterate_protocol (cd->game_state, recvline);
    cd->recv_waiting = 0;
    free (recvline);
    calculate_area(cd->game_state);
    }
  }

  pthread_mutex_unlock (cd->draw_lock);
  return 1;
}

int client_broadcast (ClientData *cd) {
  pthread_mutex_lock (cd->slice_lock);
  // Only send a command if a command is waiting
  if (cd->send_waiting) {
    client_send (cd);
    cd->send_waiting = 0;
    free (cd->command);
    cd->command = (char *) calloc (200, sizeof(char));
  }

  pthread_mutex_unlock (cd->slice_lock);
  return 1;
}

// The internal function called by the wrapper
int client_handle_communication (ClientData *cd){
  // Additional logic for locks and sleeps
  client_broadcast (cd);
  usleep(10000);
  client_update (cd);
  return 1;
}

void client_data_destroy (ClientData *cd) {
  close (cd->sockfd);
  free (cd->command);
  free (cd);
}

#ifdef DEBUG
	int main (int argc, char *argv[]) {
		//ClientData *cd = client_data_new (argv[1]);
		// if (cd != NULL) {
		// 	printf("SUCCESS!");
		// 	return EXIT_SUCCESS;
		// }

		return EXIT_FAILURE;
	}
#endif
