/********
CSC-213 Final Project Implementation: Poker Game
Authors: James Lim and Shloke Meresh
email: limjames@grinnell.edu
       mereshsh@grinnell.edu
********/

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "poker_functions.h"

// Header file created by Charlie Curtsinger
// To set up a socket and handle connections
#include "socket.h"

// Poker hand evaluator library
// Sourced from https://github.com/HenryRLee/PokerHandEvaluator
#include <phevaluator/phevaluator.h>
#include <phevaluator/rank.h>

// Max number of players in the hand - can be changed to any number
#define MAX_PLAYERS 4

// Initialize number of players and array to hold client file descriptors
int NUM_PLAYERS = 0;
int fd_arr[MAX_PLAYERS] = {0};

// Struct to store thread args
typedef struct thread_args{
  int fd;
  int pos;
} thread_args_t;

/**** Global Variables, Locks, and Condition Variables for Game State ****/

// Lock for deck
pthread_mutex_t deck_lock = PTHREAD_MUTEX_INITIALIZER;

// Keep track of which player's (which thread's) turn it is
int turn_order = 0;
pthread_mutex_t turn_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t turn_cond = PTHREAD_COND_INITIALIZER;

// Keep track of the amount in the poker pot
int pot = 0;
pthread_mutex_t pot_lock = PTHREAD_MUTEX_INITIALIZER;

// Keep track of whether the community cards have been dealt
bool community_dealt = false;
pthread_mutex_t community_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t community_cond = PTHREAD_COND_INITIALIZER;

// Keep track of whether a hand is over
bool hand_over = false;
pthread_mutex_t hand_over_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t hand_over_cond = PTHREAD_COND_INITIALIZER;

// Number of active (non-folded) players
int active_players;
pthread_mutex_t active_lock = PTHREAD_MUTEX_INITIALIZER;

// Count of players hands evaluated
int evaluated_count = 0;
pthread_mutex_t evaluated_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t evaluated_cond = PTHREAD_COND_INITIALIZER;

// Keep track of how many players' ante has been collected
int ante_count = 0;
pthread_mutex_t ante_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ante_cond = PTHREAD_COND_INITIALIZER;

// Array to hold ranks of player's hands
int rank_arr[MAX_PLAYERS];
pthread_mutex_t arr_lock = PTHREAD_MUTEX_INITIALIZER;

// Store highest rank
// Initializing to any large number (greater than rank of worst possible hand)
int best_rank = 1000000;
pthread_mutex_t best_lock = PTHREAD_MUTEX_INITIALIZER;

// Keep track of betting round
int global_betting_round = 0;
pthread_mutex_t global_bet_lock = PTHREAD_MUTEX_INITIALIZER;

// Boolean to check whether the winner of a round has been declared
bool winner_declared = false;
pthread_mutex_t winner_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t winner_cond = PTHREAD_COND_INITIALIZER;

// Number of players who have checked
int check_count = 0;
pthread_mutex_t check_lock = PTHREAD_MUTEX_INITIALIZER;

// Number of players who have called
int call_count = 0;
pthread_mutex_t call_lock = PTHREAD_MUTEX_INITIALIZER;

// Keep track of current bet
int current_bet = 0;
pthread_mutex_t current_lock = PTHREAD_MUTEX_INITIALIZER;

/***********************/

// Thread function to communicate with each individual player 
void* talk(void* arg) {

  //Unpack arguments
  thread_args_t args = *(thread_args_t*)arg;
  free(arg);
  int fd = args.fd;
  int pos = args.pos;

  // Variable to store each player's virtual cash
  int vc = 1000;

  // Tell all players we are waiting for a complete table
  if(NUM_PLAYERS < MAX_PLAYERS){
    send_message(fd, "Waiting for players.");
  } 

  // Wait until all players have joined
  while(NUM_PLAYERS < MAX_PLAYERS){
    continue;
  }

  send_message(fd, "Starting the game.");

  // Loop forever
  while (true) {

    // Set turn order to 0
    pthread_mutex_lock(&turn_lock);
    turn_order = 0;
    pthread_mutex_unlock(&turn_lock);

    // Set amount in pot to 0 
    pthread_mutex_lock(&pot_lock);
    pot = 0;
    pthread_mutex_unlock(&pot_lock);

    // Set community_dealt to false to re-deal community cards for next hand
    pthread_mutex_lock(&community_lock);
    community_dealt = false;  
    pthread_mutex_unlock(&community_lock);

    // Set hand over to false
    pthread_mutex_lock(&hand_over_lock);
    hand_over = false;
    pthread_mutex_unlock(&hand_over_lock);

    // Set number of active players to be all the players
    pthread_mutex_lock(&active_lock);
    active_players = MAX_PLAYERS;
    pthread_mutex_unlock(&active_lock);

    // Set count of evaluated hands to 0
    pthread_mutex_lock(&evaluated_lock);
    evaluated_count = 0;
    pthread_mutex_unlock(&evaluated_lock);

    // Set count of players' antes to 0
    pthread_mutex_lock(&ante_lock);
    ante_count = 0;
    pthread_mutex_unlock(&ante_lock);

    // Initialize evaluated hand rank array
    pthread_mutex_lock(&arr_lock);
    for (int i = 0; i < MAX_PLAYERS; i++){
      rank_arr[i] = 0;
    }
    pthread_mutex_unlock(&arr_lock);

    // Initialize best rank 
    pthread_mutex_lock(&best_lock);
    best_rank = 1000000;
    pthread_mutex_unlock(&best_lock);

    // Initialize global betting round
    pthread_mutex_lock(&global_bet_lock);
    global_betting_round = 0;
    pthread_mutex_unlock(&global_bet_lock);

    // Winner declared
    pthread_mutex_lock(&winner_lock);
    winner_declared = false;
    pthread_mutex_unlock(&winner_lock);

    // Reset number of players who have checked
    pthread_mutex_lock(&check_lock);
    check_count = 0;
    pthread_mutex_unlock(&check_lock);

    // Reset number of players who have called
    pthread_mutex_lock(&call_lock);
    call_count = 0;
    pthread_mutex_unlock(&call_lock);

    // Rest current bet
    pthread_mutex_lock(&current_lock);
    current_bet = 0;
    pthread_mutex_unlock(&current_lock);

    // If this is the first thread,
    if (pos == 0){

      // Initialize the deck and deal the community cards
      pthread_mutex_lock(&deck_lock);
      deck_init();
      community = deal_community();
      pthread_mutex_unlock(&deck_lock);

      // Let all waiting threads know that this has been done 
      pthread_mutex_lock(&community_lock);
      community_dealt = true;
      pthread_cond_broadcast(&community_cond);
      pthread_mutex_unlock(&community_lock);
    } else {

      // Have all other threads wait until this is done
      pthread_mutex_lock(&community_lock);
      while (!community_dealt){
        pthread_cond_wait(&community_cond, &community_lock);
      }
      pthread_mutex_unlock(&community_lock);
    }

    // Display player's virtual cash and amount in pot
    char* display_string1 = malloc(50*sizeof(char));    
    sprintf(display_string1, " \t\t [Virtual cash: $%d \t\t Amount in Pot: $0]",
                                vc);
    send_message(fd, display_string1);

    // New hand begins
    send_message(fd, "Starting a new hand.");   

    // Sleep for a second
    sleep(1);

    // Deal hand
    pthread_mutex_lock(&deck_lock);
    hand_t hand = deal_hand();
    card_t card1 = hand.card[0];
    card_t card2 = hand.card[1];
    pthread_mutex_unlock(&deck_lock);

    // Debugging code
    printf("card1: %d\n", card1.total);
    printf("card2: %d\n", card2.total);

    // Convert card values to string
    char* card1_msg = card_to_string(card1);
    char* card2_msg = card_to_string(card2);
    char* hand_string = malloc((strlen(card1_msg)+strlen(card2_msg)+35)*sizeof(char));
    sprintf(hand_string, "You have been dealt the %s and the %s.",
                          card1_msg, card2_msg);

    // Free allocated memory
    free(card1_msg);
    free(card2_msg);
    
    // String to store betting round current bet and player's bet
    char* display_string2 = malloc(44*sizeof(char));

    // Tell the player which cards have they been dealt
    send_message(fd, hand_string);

    // Free memory
    free(hand_string);

    // Boolean for whether or not this player has folded
    bool folded = false;

    // Counter for which betting round it is
    int betting_round = 0;

    // Store player's bet for each betting round
    int your_bet = 0;

    // Loop as many times as there are betting rounds
    while (betting_round < 5){

      // Debugging code
      printf("\nbetting round: %d\n", betting_round);

      // If we are preflop
      if (betting_round == 0){

        // Take ante, decrement virtual cash, increment pot
        send_message(fd, "Taking $10 as ante."); 
        vc -= 10;
        pthread_mutex_lock(&pot_lock);
        pot += 10;
        pthread_mutex_unlock(&pot_lock);

        // Increment number of players whose ante has been taken
        pthread_mutex_lock(&ante_lock);
        ante_count++;
        pthread_cond_broadcast(&ante_cond);
        pthread_mutex_unlock(&ante_lock);

        // Move this player to the next betting round
        betting_round++;

        // Go to next iteration of loop
        continue;
      }

      // If ante has been taken
      if (betting_round == 1){

        // Block thread and wait until all players' ante has been taken 
        pthread_mutex_lock(&ante_lock);
        while (ante_count < active_players){
          pthread_cond_wait(&ante_cond, &ante_lock);
        }
        pthread_mutex_unlock(&ante_lock);

        // All ante collected, set global betting round to 1
        pthread_mutex_lock(&global_bet_lock);
        global_betting_round = 1;
        pthread_mutex_unlock(&global_bet_lock);
      }

      // Block thread and wait while it is not your turn
      pthread_mutex_lock(&turn_lock);
      while (turn_order != pos){
        // Unless player has folded, player is awaiting its turn 
        if (!folded){
          send_message(fd, "Awaiting turn.");
        }
        pthread_cond_wait(&turn_cond, &turn_lock);

        // Debugging code
        printf("Turn order: %d\n", turn_order);
      }
      pthread_mutex_unlock(&turn_lock);

      // Make sure that all threads are on the same betting round
      pthread_mutex_lock(&global_bet_lock);
      betting_round = global_betting_round;

      // Check if we have gone through all the betting rounds
      if(global_betting_round == 4 && check_count == active_players){

        // If yes, go to the next player and then break out of the betting loop
        pthread_mutex_lock(&turn_lock);
        turn_order++;
        if (turn_order == MAX_PLAYERS){
          turn_order = 0;
        }
        pthread_cond_broadcast(&turn_cond);
        pthread_mutex_unlock(&turn_lock);

        // Unlock global betting round
        pthread_mutex_unlock(&global_bet_lock);

        break;
      }

      // Unlock global betting round
      pthread_mutex_unlock(&global_bet_lock);

      // If this player has folded, skip them
      if (folded){
        pthread_mutex_lock(&turn_lock);
          turn_order++;
          if (turn_order == MAX_PLAYERS){
            turn_order = 0;
          }
        pthread_cond_broadcast(&turn_cond);
        pthread_mutex_unlock(&turn_lock);

        continue;
      }

      // If it is their turn, send message to client
      send_message(fd, "It is your turn. Do you want to fold, check, call or raise?");
      
      // Display player's virtual cash and amount in pot
      sprintf(display_string1, " \t\t [Virtual cash: $%d \t\t Amount in Pot: $%d]",
                                  vc, pot);
      send_message(fd, display_string1);

      // Display current bet of round and player's bet
      sprintf(display_string2, " \t\t [Current bet: $%d \t\t Your bet: $%d]",
                                  current_bet, your_bet);
      send_message(fd, display_string2);
      
      // Send a message indicating it is that player's turn
      send_message(fd, "indicate_turn");

      // Message to be received from client
      char* message;

      // Receive a message from the client
      while (true){
        message = receive_message(fd);

        // Check for errors
        if (message == NULL){
          for (int i = 0; i < MAX_PLAYERS; i++){
            if (fd_arr[i] != fd){
              send_message(fd, "A player has exited the table. The program is terminating.");
            }
          }
          exit(2);
        }
        
        break;
      }

      // string to store player's move
      char action[8];

      // Go through the received message backwards to find out what the player's move was
      for (int i = 0; i < 8; i++){

        // Add null character at the end
        if (i == 7){
          action[i] = '\0';
        } else {
          action[i] = message[strlen(message)-7+i];
        }

      }

      // If the player folds
      if(strcmp(action, " folds.") == 0){

        send_message(fd, "You have folded. You must wait till the next hand.");

        // Tell all other players this player folded
        for (int i = 0; i < MAX_PLAYERS; i++){
          if (fd_arr[i] != fd){
            send_message(fd_arr[i], message);
          }
        }

        // Set folded to true
        folded = true;

        // If all player have checked or called
        if (check_count == active_players || call_count == active_players){

          // Change your bet to 0
          your_bet = 0;

          // Change current bet to 0
          current_bet = 0;

          // Change call count and check count to 0
          call_count = 0;
          check_count = 0;
          
          // Get appropriate string of community cards
          char* community_string = flop_turn_river(betting_round);

          // Send community string to all players
          for(int i = 0; i < MAX_PLAYERS; i++){
            send_message(fd_arr[i], community_string);
          }

          // Free memory malloced in flop_turn_river function call
          free(community_string);

          // Go to next betting round
          betting_round++;

          // Set global betting round to be the current betting round
          pthread_mutex_lock(&global_bet_lock);
          global_betting_round = betting_round;
          pthread_mutex_unlock(&global_bet_lock);
        }

        // Decrement number of active players
        active_players--;

        // Go to the next player
        pthread_mutex_lock(&turn_lock);
        turn_order++;
        if (turn_order == MAX_PLAYERS){
          turn_order = 0;
        }
        pthread_cond_broadcast(&turn_cond);
        pthread_mutex_unlock(&turn_lock);

        continue;
      } 

      // If player raises
      else if(strcmp(action, "raises.") == 0) {

        send_message(fd, "How much do you want to raise to?");
        send_message(fd, "indicate_raise");

        // String to get raise amount
        char* raise_amount_string;

        while (true){

          // Get the amount the player wants to raise to
          raise_amount_string = receive_message(fd);

          // Check for errors
          if (message == NULL){
            for (int i = 0; i < MAX_PLAYERS; i++){
              if (fd_arr[i] != fd){
                send_message(fd, "A player has exited the table. The program is terminating.");
              }
            }
            exit(2);
          }

          break;

        }

        // Convert it to an integer
        int raise_amount = atoi(raise_amount_string);

        // Free raise amount string
        free(raise_amount_string);

        // Calculate amount the player wants to raise their current bet by
        int difference = raise_amount - your_bet;

        // If this amount is greater than their virtual cash,
        // Send appropriate messages and 
        // Skip to next iteration of loop to ask for input again
        if (difference > vc){
          send_message(fd, "Don't bet your car keys! Your raise amount exceeds your virtual cash.\n");
          send_message(fd, "Let's try again.");
          free(raise_amount_string);
          continue;
        }

        // If the raise amount is less than the current bet
        if (raise_amount <= current_bet){
          send_message(fd, "Raise amount must be greater than current bet.");
          send_message(fd, "Let's try again.");            
          free(raise_amount_string);
          continue;
        }

        // Generate message to send to other players
        char* message_to_send = malloc((strlen(message)+11)*sizeof(char));
        char* remove_fullstop = strndup(message, (strlen(message)-1));
        sprintf(message_to_send, "%s to $%d.", remove_fullstop, raise_amount);
        free(remove_fullstop);

        // Tell all other players this player raised
        for (int i = 0; i < MAX_PLAYERS; i++){
          if (fd_arr[i] != fd){
            send_message(fd_arr[i], message_to_send);
          }
        }

        // Free allocated memory 
        free(message_to_send);

        // Change call_count to 1 since we need all other players to call
        call_count = 1;

        // Add this difference to the pot
        pthread_mutex_lock(&pot_lock);
        pot += difference;
        pthread_mutex_unlock(&pot_lock);

        // Decrement virtual cash appropriately
        vc -= difference;

        // Make this your bet
        your_bet = raise_amount;

        // Make this the new current bet
        current_bet = your_bet;

        // Go to the next player's turn
        pthread_mutex_lock(&turn_lock);
        turn_order++;
        if (turn_order == MAX_PLAYERS){
          turn_order = 0;
        }
        pthread_cond_broadcast(&turn_cond);
        pthread_mutex_unlock(&turn_lock);

      }

      // If player calls
      else if(strcmp(action, " calls.") == 0){
        
        // Calculate difference between current and player's bet
        int difference = current_bet - your_bet;

        // If they do not have enough cash to call
        if ((vc - difference) < 0){
          send_message(fd, "You do not have enough virtual cash to call.");
          send_message(fd, "Let's try again.");
          free(message);
          continue;
        }

        // If they do not need to call
        if (your_bet == current_bet){
          send_message(fd, "You can only call when another player raises.");
          send_message(fd, "Let's try again.");
          free(message);
          continue;
        }

        // Tell all other players this player called
        for (int i = 0; i < MAX_PLAYERS; i++){
          if (fd_arr[i] != fd){
            send_message(fd_arr[i], message);
          }
        }

        // Increment number of players that have called
        call_count++;

        // Add this difference to the pot
        pthread_mutex_lock(&pot_lock);
        pot += difference;
        pthread_mutex_unlock(&pot_lock);

        // Decrment virtual cash appropriately
        vc -= difference;

        // Change your bet to be the current bet
        your_bet = current_bet;

        // If all active players have called
        if (call_count == active_players){

          // Change round current bet back to 0
          current_bet = 0;

          // Change player's bet back to 0
          your_bet = 0;

          if (betting_round < 4){
            // Get appropriate string of community cards
            char* community_string = flop_turn_river(betting_round);

            // Send community string to all players
            for(int i = 0; i < MAX_PLAYERS; i++){
              send_message(fd_arr[i], community_string);
            }

            // Free memory malloced in flop_turn_river function call
            free(community_string);
          }

          else if (betting_round == 4){

            // Go to the next player's turn
            pthread_mutex_lock(&turn_lock);
            turn_order = 0;
            pthread_cond_broadcast(&turn_cond);
            pthread_mutex_unlock(&turn_lock);

            pthread_mutex_lock(&hand_over_lock);
            hand_over = true;
            pthread_cond_broadcast(&hand_over_cond);
            pthread_mutex_unlock(&hand_over_lock);

            break;

          }

          // Bring call count back to 0
          call_count = 0;

          // Bring check count back to 0
          check_count = 0;

          // Go to next betting round
          betting_round++;

          pthread_mutex_lock(&global_bet_lock);
          global_betting_round++;
          pthread_mutex_unlock(&global_bet_lock);

          // Go back to first player's turn
          pthread_mutex_lock(&turn_lock);
          turn_order = 0;
          pthread_cond_broadcast(&turn_cond);
          pthread_mutex_unlock(&turn_lock);

        } else {
          // Go to the next player's turn
          pthread_mutex_lock(&turn_lock);
          turn_order++;
          if (turn_order == MAX_PLAYERS){
            turn_order = 0;
          }
          pthread_cond_broadcast(&turn_cond);
          pthread_mutex_unlock(&turn_lock);

        }

      }

      // If player checks
      else if (strcmp(action, "checks.") == 0){

        // If player's bet is less than the current bet
        if (your_bet < current_bet){
          send_message(fd, "You cannot check until you match the current bet.");
          send_message(fd, "Let's try again.");
          free(message);
          continue;
        }

        // Tell all other players this player checked
        for (int i = 0; i < MAX_PLAYERS; i++){
          if (fd_arr[i] != fd){
            send_message(fd_arr[i], message);
          }
        }
        
        // Increment number of players that have checked
        pthread_mutex_lock(&check_lock);
        check_count++;
        pthread_mutex_unlock(&check_lock);

        // If all active players have checked
        if(check_count == active_players){

          // Change round current bet back to 0
          current_bet = 0;

          // Change player's bet back to 0
          your_bet = 0;

          if (betting_round < 4){
            // Get appropriate string of community cards
            char* community_string = flop_turn_river(betting_round);

            // Send community string to all players
            for(int i = 0; i < MAX_PLAYERS; i++){
              send_message(fd_arr[i], community_string);
            }

            // Free memory malloced in flop_turn_river function call
            free(community_string);
          }

          else if (betting_round == 4){

            // Go to the next player's turn
            pthread_mutex_lock(&turn_lock);
            turn_order = 0;
            pthread_cond_broadcast(&turn_cond);
            pthread_mutex_unlock(&turn_lock);

            pthread_mutex_lock(&hand_over_lock);
            hand_over = true;
            pthread_cond_broadcast(&hand_over_cond);
            pthread_mutex_unlock(&hand_over_lock);

            break;

          }

          // Bring check count back to 0
          check_count = 0;

          // Bring call count back to 0
          call_count = 0;

          // Go to next betting round
          betting_round++;

          pthread_mutex_lock(&global_bet_lock);
          global_betting_round++;
          pthread_mutex_unlock(&global_bet_lock);


          // Go back to first player's turn
          pthread_mutex_lock(&turn_lock);
          turn_order = 0;
          pthread_cond_broadcast(&turn_cond);
          pthread_mutex_unlock(&turn_lock);


        } else {
          // Go to the next player's turn
          pthread_mutex_lock(&turn_lock);
          turn_order++;
          if (turn_order == MAX_PLAYERS){
            turn_order = 0;
          }
          pthread_cond_broadcast(&turn_cond);
          pthread_mutex_unlock(&turn_lock);

        }

      }

      // Display player's virtual cash and amount in pot
      sprintf(display_string1, " \t\t [Virtual cash: $%d \t\t Amount in Pot: $%d]",
                                  vc, pot);
      send_message(fd, display_string1);

      // Display current bet of round and player's bet
      sprintf(display_string2, " \t\t [Current bet: $%d \t\t Your bet: $%d]",
                                  current_bet, your_bet);
      send_message(fd, display_string2);

      // Free message string
      free(message);

    }

    // Free memory malloced for display strings
    free(display_string1);
    free(display_string2);

    // Ask for username
    send_message(fd, "provide_username");

    // String to store username
    char* username;

    // Loop until we get the username
    while (true){
      username = receive_message(fd);
      if (username == NULL){
          for (int i = 0; i < MAX_PLAYERS; i++){
            if (fd_arr[i] != fd){
              send_message(fd, "A player has exited the table. The program is terminating.");
            }
          }
          exit(2);
        }
      break;
    }

    // Evaluated hand of 7 cards to get the rank of the best 5
    int hand_rank = evaluate_7cards(card1.total
                                  , card2.total
                                  , community.card[0].total
                                  , community.card[1].total
                                  , community.card[2].total
                                  , community.card[3].total
                                  , community.card[4].total);

    // If the player is folded, give them a very large rank (can never win)
    if (folded) {
      rank_arr[pos] = 100000000;
    } else {
      // Else add their hand to the rank array and increment evaluated count
      rank_arr[pos] = hand_rank;
      pthread_mutex_lock(&evaluated_lock);
      evaluated_count++;
      pthread_cond_broadcast(&evaluated_cond);
      pthread_mutex_unlock(&evaluated_lock);
    }

    // Get string describing player's hand
    const char* evaluated_string = malloc(40*sizeof(char)); 
    evaluated_string = describe_rank(hand_rank);
      
    char* final_string = malloc((strlen(evaluated_string)
                            + strlen(username)
                            + 20)*sizeof(char));

    sprintf(final_string, "%s has: %s.", username, evaluated_string);

    // If not folded
    if (!folded){

      // Send this player's hand to all players
      for (int i = 0; i < MAX_PLAYERS; i++){
        send_message(fd_arr[i], final_string);
      }

      free(final_string);

      // Figure out who has the best hand 
      pthread_mutex_lock(&best_lock);
      if (hand_rank < best_rank) {
        best_rank = hand_rank;
      }
      pthread_mutex_unlock(&best_lock);
    } 

    // Wait until all hands have been evaluated
    while (evaluated_count < active_players){
      pthread_cond_wait(&evaluated_cond, &evaluated_lock);
    }
    pthread_mutex_unlock(&evaluated_lock);

    // When all hands have been evaluated, find who wins
    if (hand_rank == best_rank){

      // Let all players know who the winner is
      char* win_message = malloc((strlen(username)+7) * sizeof(char));
      sprintf(win_message, "%s wins!", username);
      for (int i = 0; i < MAX_PLAYERS; i++){
        send_message(fd_arr[i], win_message);
      }

      // Free malloced memory 
      free(win_message);

      // Increase winner's vc by amount in pot
      pthread_mutex_lock(&pot_lock);
      vc += pot;
      pthread_mutex_unlock(&pot_lock);

      // Let all players know winner has been declared
      pthread_mutex_lock(&winner_lock);
      winner_declared = true;
      pthread_cond_broadcast(&winner_cond);
      pthread_mutex_unlock(&winner_lock);
    } else {

      // Wait for winner to be declared
      pthread_mutex_lock(&winner_lock);
      while (!winner_declared){
        pthread_cond_wait(&winner_cond, &winner_lock);
      }
      pthread_mutex_unlock(&winner_lock);
    }

    // Free username string
    free(username);

    // Sleep for a second
    sleep(1);
 
  }

  return NULL;
}


// Main function 
int main(int argc, char** argv){

  // Open a server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(2);
  }

  printf("Listening on port %d\n", port);

  // Start listening for connections, with a maximum of one queued connection
  if (listen(server_socket_fd, 1)) {
    perror("listen failed");
    exit(2);
  }

  // Initialize deck and community cards
  deck_init();
  community_init();

  // Loop until all player positions are filled
  while (NUM_PLAYERS < MAX_PLAYERS) {

    // Wait for a client to connect
    int fd = server_socket_accept(server_socket_fd);
    if (fd == -1) {
      perror("accept failed");
      exit(2);
    }

    // Add fd of new player to fd_arr
    fd_arr[NUM_PLAYERS] = fd;

    // Create thread args to pass to thread function
    thread_args_t* args = malloc(sizeof(thread_args_t));
    args->fd = fd;
    args->pos = NUM_PLAYERS;

    // Create a thread to talk to the new player
    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, talk, args)) {
      perror("pthread_create failed");
      exit(EXIT_FAILURE);
    }

    // Increment number of players
    NUM_PLAYERS++;
  }

  // Main thread loops forever
  while(1){};

  return 0;
}