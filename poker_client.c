#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "socket.h"
#include "poker_functions.h"

#define MESSAGE_LENGTH 256


typedef struct thread_args {
  int fd;
  char* username;
} thread_args_t;


void* talk (void* arg){

  int vc = 1000;
  char cash[256];

  // Unpack arguments
  thread_args_t args = *(thread_args_t*)arg;
  free(arg);

  char* username = strdup(args.username);

  int fd = args.fd;

  printf("%s\n", args.username);

  bool client_alive = true;
  while (client_alive) {
    // Read a message from the client
    char* message_received = receive_message(fd);
    
    if (message_received == NULL){
      exit(1);
    }

    if (strcmp(message_received, "indicate_turn") == 0){
      
      //printf("%s\n", message_received);

      // Read messages from the local user
      char buffer[MESSAGE_LENGTH];

      // Create string to hold message to be sent to server
      char* message_to_send = malloc((strlen(username) + 8)*sizeof(char));

      while (fgets(buffer, MESSAGE_LENGTH, stdin) != NULL) {

        if (strcmp(buffer, "fold\n") == 0){

          sprintf(message_to_send, "%s folds.", username);

        } 
        else if (strcmp(buffer, "check\n") == 0){

          sprintf(message_to_send, "%s checks.", username);

        } 
        else if (strcmp(buffer, "call\n") == 0){
          
          sprintf(message_to_send, "%s calls.", username);

        } 
        else if (strcmp(buffer, "raise\n") == 0){
          
          sprintf(message_to_send, "%s raises.", username);

        } else {

          printf("Your options are: fold, check, call, or raise.\n");
          continue;  

        }

        // Send the message
        send_message(fd, message_to_send);
        break;
      }
    } 
    
    else if (strcmp(message_received, "indicate_raise") == 0){

      // Read input from local user
      char raise_amount[MESSAGE_LENGTH];
      while (fgets(raise_amount, MESSAGE_LENGTH, stdin) != NULL) {
        // Send it to server
        send_message(fd, raise_amount);
        break;
      }

    } 

    else if (strcmp(message_received, "provide_username") == 0){

      // Send username
      send_message(fd, username);

    }
    
    else {
      // Print the message and repeat
      printf("%s\n", message_received);
    }

  }

  return NULL;
}

int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <> <server name> <port>\n", argv[0]);
    exit(1);
  }

  // Read command line arguments
  char* player_name = argv[1];
  char* server_name = argv[2];
  unsigned short port = atoi(argv[3]);

  // Connect to the server
  int socket_fd = socket_connect(server_name, port);
  if (socket_fd == -1) {
    perror("Failed to connect");
    exit(2);
  }

  //pthread_t send_thread;
  pthread_t receive_thread;

  // Create int pointer to fd to pass to thread function
  int* arg = malloc(sizeof(int));
  *arg = socket_fd;

  thread_args_t* args = malloc(sizeof(thread_args_t));
  args->fd = socket_fd;
  args->username = strdup(player_name);

  if (pthread_create(&receive_thread, NULL, talk, args)) {
    perror("pthread_create failed");
    exit(EXIT_FAILURE);
  }

  while(1){};

  // Close socket
  close(socket_fd);

  return 0;
}