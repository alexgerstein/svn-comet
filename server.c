/* Server.c */

/*
  Each client sends "END"
  Server increments END counter
  If END counter == 2
  clear and remake spaces on server
  call core_load_level
  cd->sendline must be load level, pass to send




*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "physics.h"
#include "core.h"

#define TIMESTEP (1.0/100)
#define SERVER_PORT 3002
#define LISTENQ 2           // maximum number of client connections
#define MAXLINE 4096 
#define MAX_ITERATIONS 5 
#define MAX_LEVELS 3

/* Structure to store information about the server */
  typedef struct {
  	GameState *gm_st;
  	int listenfd;
  	int newfd;
  	int fdmax;

  	char *level_info;

  	int end_count;
  	int iterations;

  	fd_set master;
  	fd_set readfds;

  	struct sockaddr_in serveraddr;
  	int *connfd;
  	int num_conns;
  	int close;
  /* char ** to store the next command */
  	char *command;

  	int slices_1;
  	int slices_2;

  } ServerData;

//New prototypes to handle the loop
  static void server_loop (ServerData *s_data);
  static int server_initialize (ServerData *s_data);


  static ServerData *server_data_new ();
  static void server_data_destroy (ServerData *s_data);
/* Function that blocks until it receives the next command from the board */
  static int server_receive (ServerData *s_data, int i);
/* Function that sends the next command to all the users */
  static int server_send (ServerData *s_data);

  int main (int argc, char **argv){
  	ServerData *s_data = server_data_new ();

  	FD_ZERO (&s_data->readfds);
  	FD_ZERO (&s_data->master);


  	listen (s_data->listenfd, LISTENQ);
  	FD_SET (s_data->listenfd, &(s_data->master));
  	s_data->fdmax = s_data->listenfd;

  	while (s_data->close == 0){
  		server_loop (s_data);
  	}

  	close (s_data->listenfd);
  	close ((s_data->connfd)[0]);

  	server_data_destroy(s_data);
  	return EXIT_SUCCESS;

  }

  static void server_loop (ServerData *s_data) {
  	struct timeval tv = {0, 20000};

  	s_data->readfds = s_data->master;

  //printf("ENTERING SELECT\n");

  	if (select (s_data->fdmax + 1, &s_data->readfds, NULL, NULL, &tv) == -1) {
  		perror ("select");
  		exit(4);
  	}

  //printf("EXITED SELECT\n");

  	for (int i = 0; i <= s_data->fdmax; i++) {
  		if (FD_ISSET(i, &s_data->readfds)) {
  			if (i == s_data->listenfd) {
        //printf("IN SERVER INITIALIZE\n");
  				if (s_data->num_conns < 2) {
  					server_initialize (s_data);
  				}
        //server_send (s_data);
        //printf("EXITED SERVER INITIALIZE\n");
  			}  
  			else {
        //printf("IN SERVER RECEIVE\n");
  				server_receive (s_data, i);
        //printf("EXITED SERVER RECEIVE\n");
  			}
  		}
  	}

  	if (s_data->num_conns > 1) {
  		if (s_data->iterations >= MAX_ITERATIONS) {
  			server_send (s_data);
  			s_data->iterations = 0;
  		}
      if (s_data->gm_st->space1 != NULL) {
  		  cpSpaceStep(s_data->gm_st->space1, s_data->gm_st->dt);
  		  check_shapes(s_data->gm_st->space1);
      }
      if (s_data->gm_st->space2 != NULL) {
        cpSpaceStep(s_data->gm_st->space2, s_data->gm_st->dt);
    		check_shapes(s_data->gm_st->space2);
      }
  		s_data->iterations += 1;
    //printf("IN SERVER SEND\n");
  	}

  //printf("EXITED SERVER SEND\n");

  // Function that quickly checks for new connections, and if none, returns
  // printf("REENTERED INITIALIZE\n");

// Function that takes the next message to be sent out (usually a space update) and sends it to all current clients. Function guarantees that all clients have received the message until we return (maybe a better way to do it)
  // printf("EXITTED INITIALIZE\n");

  // cpSpaceUpdate(s_data); // Update the space so that we can send out data
  // char **command;
  // createUpdateCommand(s_data, command);   // Generate an update command that can be sent to the clients. This command should be stored in the server_data struct so that it can be sent out
  }

  static int server_initialize (ServerData *s_data) {
  	struct sockaddr_in cliaddr;
  	printf("FD IS SET!!!\n");
  	socklen_t clilen = sizeof(cliaddr);
  	s_data->newfd = accept (s_data->listenfd, (struct sockaddr *) &cliaddr, &clilen);

  	if (s_data->newfd == -1) {
  		perror("accept");
  	} else {
  		FD_SET (s_data->newfd, &s_data->master);
  		s_data->connfd[s_data->num_conns] = s_data->newfd;

  		if (s_data->num_conns == 0) {
  			s_data->gm_st->space1 = physics_create_space (s_data->newfd);
  		}
  		else if (s_data->num_conns == 1) {
  			s_data->gm_st->space2 = physics_create_space (s_data->newfd);
  			core_iterate_protocol (s_data->gm_st, s_data->level_info);
  		}

  		s_data->num_conns += 1;
  		if (s_data->newfd > s_data->fdmax) {
  			s_data->fdmax = s_data->newfd;
  		}
  	}

  	return 1;
  }

  static int server_receive (ServerData *s_data, int i) {
  	char *recvline = (char *) calloc (200, sizeof(char));

  	int n;
  	if ((n = recv(i, recvline, 199, 0)) <= 0) {
  		if (n == 0) {
  			perror("Lost connection to client. Resetting server.\n");
  		} else {
  			perror("recv");
  		}

    //MAYBE KEEP
    // cpSpace *space_to_remove;
    // if (s_data->connfd[0] == i) {
    //   space_to_remove = s_data->gm_st->space1;
    // } else if (s_data->connfd[1] == i) {
    //   space_to_remove = s_data->gm_st->space2;
    // }

  		cpSpaceFree(s_data->gm_st->space1);
  		cpSpaceFree(s_data->gm_st->space2);
  		s_data->gm_st->space1 = NULL;
  		s_data->gm_st->space2 = NULL;
  		s_data->gm_st->level = 1;

  		if (s_data->connfd[0] != 0) {
  			close (s_data->connfd[0]);
  			FD_CLR(s_data->connfd[0], &s_data->master);
  			s_data->connfd[0] = 0;
  		}
  		if (s_data->connfd[1] != 0) {
  			close (s_data->connfd[1]);
  			FD_CLR(s_data->connfd[1], &s_data->master);
  			s_data->connfd[1] = 0;
  		}

  		s_data->num_conns = 0;
  		free(s_data->level_info);
  		s_data->level_info = core_load_level (s_data->gm_st, s_data->gm_st->level);
  	} else {
  		printf ("SERVER RECEIEVED: %s\n", recvline);

  		if (recvline[0] == 'E') {
  			s_data->end_count += 1;
  			printf("End counter -> %d \n", s_data->end_count);

  			if (s_data->end_count == 2) {
          s_data->gm_st->orig_area = 0;
          s_data->iterations = 0;
          int new_level = s_data->gm_st->level += 1;

          s_data->end_count = 0;
    			cpSpaceFree(s_data->gm_st->space1);
    			cpSpaceFree(s_data->gm_st->space2);
    			s_data->gm_st->space1 = physics_create_space(s_data->connfd[0]);
    			s_data->gm_st->space2 = physics_create_space(s_data->connfd[1]);
          free(s_data->level_info);
          s_data->level_info = core_load_level(s_data->gm_st, s_data->gm_st->level);
        }
  		}

  		else {

  			cpSpace *space_to_slice;
  			if (s_data->connfd[0] == i) {
  				space_to_slice = s_data->gm_st->space1;

  			} else if (s_data->connfd[1] == i) {
  				space_to_slice = s_data->gm_st->space2;

  			} else {
  				return 0;
  			}

  			if(space_to_slice == s_data->gm_st->space1 &&  core_server_handle_slice(space_to_slice, recvline)) {
  				s_data->slices_1 -= 1;
  				send (i," 0 SLICED \n", 11, 0);
  			}

  			else if (space_to_slice == s_data->gm_st->space2 &&  core_server_handle_slice(space_to_slice, recvline)) {
  				s_data->slices_2 -= 1;
  				send (i," 0 SLICED \n", 11, 0);
  			}
  		}
      free(recvline);
  	}
  	return 1;
  }

  static int server_send (ServerData *s_data) {

  	if (s_data->command == NULL) {
  		return 0;
  	}

  	errno = 0;
  	for (int p = 0; p < 2; p++) {
  		for (int i = 0; i <= s_data->fdmax; i++) {
  			if (FD_ISSET (i, &s_data->master)) {
  				if (i != s_data->listenfd) {

  					cpSpace *space_to_update = s_data->gm_st->space1;
  					if (p == 1 && s_data->gm_st->space2 != NULL) {
  						space_to_update = s_data->gm_st->space2;
  					}
            core_server_generate_updates (s_data->command, space_to_update, i);
  					int n;

            if (s_data->command == NULL) {
              continue;
            }

  					if ((n = send (i, s_data->command, strlen(s_data->command), 0)) == -1) {
  						perror("send");
  					}
          // if (n == 0) {
          //   return 1;
          // }
  					if (n != 0) {
              memset(s_data->command, 0, sizeof(char) * 5000);
            }
  				}
  			}
  		}
  	}
  	return 1;
  }

//   int *recv_status = (int *) malloc (s_data->num_conns * sizeof(int));
//   // printf("STEPPED INTO SEND\n");
//   for (int i = 0; i < s_data->num_conns; i++) { recv_status[i] = 0; }
//   int all_sent = 0;
//   // Loop guarantees that all clients have received the updated data
//   while (!all_sent) {
//     // Set it now to 1 so that if all the recv_statuses are 1, it remains 1 and exits
//     // printf("Not sent to all connections. Sending to next client.\n");
//     all_sent = 1;
//     // Create a new fd_set and only include those file descriptors that have not received yet
//     fd_set readfds;
//     FD_ZERO (&readfds);
//     for (int i = 0; i < s_data->num_conns; i++) { 
//       if (recv_status[i] == 0) { FD_SET ((s_data->connfd)[i], &readfds); }
//     }

//     select ((s_data->connfd)[s_data->num_conns-1]+1, &readfds, NULL, NULL, NULL);
//     for (int i = 0; i < s_data->num_conns; i++) {
//       // Exits immediately if we have already sent
//       if (recv_status[i] == 0 && FD_ISSET ((s_data->connfd)[i], &readfds)) {
//       	const char* command = "Sent data.";
//       	//send ((s_data->connfds)[i], s_data->send_command, strlen(s_data->send_command), NULL);
//       	send ((s_data->connfd)[i], command, strlen(command), NULL);
//         printf("Sent data to client number %d\n", i);
//       	recv_status[i] = 1;
//       }
//       all_sent = all_sent && recv_status[i];  // When all files have been sent, evaluates to 1 and we exit loop 
//     }
//   }
//   return 1;
// }

/* Create a socket data structure that performs initialization */
  static ServerData *server_data_new () {
  	ServerData *s_data = (ServerData *) malloc (sizeof(ServerData));
  	s_data->gm_st = game_state_new();

  	s_data->level_info = core_load_level (s_data->gm_st, s_data->gm_st->level);

  	s_data->listenfd = socket (AF_INET, SOCK_STREAM, 0);
  	s_data->connfd = (int *) malloc (2 * sizeof(int));
  	s_data->connfd[0] = 0;
  	s_data->connfd[1] = 0;
  /* Initialize the server sock_addr pointer */
  	(s_data->serveraddr).sin_family = AF_INET;
  	(s_data->serveraddr).sin_addr.s_addr = htonl(INADDR_ANY); 
  	(s_data->serveraddr).sin_port = htons(SERVER_PORT);
  	bind (s_data->listenfd, (struct sockaddr*) &(s_data->serveraddr), sizeof(s_data->serveraddr));
  	s_data->close = 0;
  	s_data->num_conns = 0;

  	s_data->command = (char *) calloc (5000, sizeof(char));
  	return s_data;
  }

  static void server_data_destroy (ServerData *s_data){
    // game_state_destroy (s_data->gm_st);
  	close (s_data->listenfd);
  	close ((s_data->connfd)[0]);
  	close ((s_data->connfd)[1]);
  	free (s_data->gm_st);
  	free (s_data);
  }


