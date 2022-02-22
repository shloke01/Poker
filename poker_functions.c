#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <phevaluator/phevaluator.h>
#include <phevaluator/rank.h>
#include <string.h>

#include "poker_functions.h"

// Taken from Charlie Curtsinger's worm lab
// Returns time in miliseconds
size_t time_ms() {
  struct timeval tv;
  if (gettimeofday(&tv, NULL) == -1) {
    perror("gettimeofday");
    exit(2);
  }
  // Convert timeval values to milliseconds
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// Convert card to be displayed as a string
char* card_to_string(card_t card){
  char val_string[6];
  char suit_string[9];

  switch (card.val){
    case 0:
      strcpy(val_string, "Two");
      break;
    case 1:
      strcpy(val_string, "Three");
      break;
    case 2:
      strcpy(val_string, "Four");
      break;
    case 3:
      strcpy(val_string, "Five");
      break;
    case 4:
      strcpy(val_string, "Six");
      break;
    case 5:
      strcpy(val_string, "Seven");
      break;
    case 6:
      strcpy(val_string, "Eight");
      break;
    case 7:
      strcpy(val_string, "Nine");
      break;
    case 8:
      strcpy(val_string, "Ten");
      break;
    case 9:
      strcpy(val_string, "Jack");
      break;
    case 10:
      strcpy(val_string, "Queen");
      break;
    case 11:
      strcpy(val_string, "King");
      break;
    case 12:
      strcpy(val_string, "Ace");
      break;
  }

  // clubs = 0
  // diammonds = 1
  // hearts = 2
  // spades = 3

  switch(card.suit){
    case 0:
      strcpy(suit_string, "Clubs");
      break;
    case 1:
      strcpy(suit_string, "Diamonds");
      break;
    case 2:
      strcpy(suit_string, "Hearts");
      break;
    case 3:
      strcpy(suit_string, "Spades");
      break;
  }

  char* card_string = malloc((strlen(val_string)  +
                              strlen(" of ") +
                              strlen(suit_string)  + 1)*sizeof(char));

  sprintf(card_string, "%s of %s", val_string, suit_string);

  printf("%s\n", card_string);

  return card_string;
}

// Initialize deck to full deck of 52 cards (no jokers)
void deck_init(){
  for(int i = 0; i < 52; i++){
    deck.card[i] = i;
  }
  return;
}

// Initialize community cards to 0s
void community_init(){
  for(int i = 0; i < 5; i++){
    community.card[i].total = 0;
  }
  return;
}

// Remove a given card from the deck
void remove_card_from_deck(int card){
  deck.card[card] = -1;
  return;
}

// Generate a random card from a deck of 52 cards
card_t rand_card(){

  card_t new_card;

  // Seed random time and get random number from 0 to 51
  srand(time_ms());
  int random_number = rand()%52;

  // Generate a new random number till we get one that is present in the deck
  while(deck.card[random_number] == -1){
    random_number = rand()%52;
  }

  // Assign this value to the new card and remove this card from the deck
  new_card.total = random_number;
  remove_card_from_deck(new_card.total);

  // Assign appropriate card suit and card val
  new_card.suit = new_card.total%4;
  new_card.val = (new_card.total - new_card.suit)/4;

  return new_card;
}

// Function to print the deck
void print_deck(){
  for (int i = 0; i < 52; i++){
    printf("%d\t", deck.card[i]);
    if (i == 51){
      printf("\n");
    }
  }
}

// Deal a hand of two cards
hand_t deal_hand(){
  hand_t hand;
  hand.card[0] = rand_card();
  //printf("You have been dealt %d\n", rand_card().total);
  //print_deck();
  hand.card[1] = rand_card();
  //printf("You have been dealt %d\n", rand_card().total);
  return hand;
}

// deal the community cards
community_t deal_community(){
  community_t community;
  for(int i = 0; i < 5; i++){
    community.card[i] = rand_card();
  }
  return community;
}

// Get a string of the appropriate community cards
char* flop_turn_river(int betting_round){

  // Store all the community cards as strings
  char* com_card1 = card_to_string(community.card[0]); 
  char* com_card2 = card_to_string(community.card[1]); 
  char* com_card3 = card_to_string(community.card[2]); 
  char* com_card4 = card_to_string(community.card[3]); 
  char* com_card5 = card_to_string(community.card[4]); 

  // Allocate space to store the necessary string
  char* community_string = malloc((strlen(com_card1) 
                              + strlen(com_card2) 
                              + strlen(com_card3)
                              + strlen(com_card4)
                              + strlen(com_card5) 
                              + 20) * sizeof(char));

  // Function to do this repeated code
  if(betting_round == 1) {

    // Store the flop string
    sprintf(community_string, "Flop: %s | %s | %s",
            com_card1, com_card2, com_card3);

  } else if(betting_round == 2) {

    // Store the turn string
    sprintf(community_string, "Turn: %s | %s | %s | %s" ,
            com_card1, com_card2, com_card3, com_card4);

  } else if(betting_round == 3) {

    // Store the river string
    sprintf(community_string, "River: %s | %s | %s | %s | %s" ,
            com_card1, com_card2, com_card3, com_card4, com_card5);

  }

  // Free memory allocated in card_to_string calls
  free(com_card1);
  free(com_card2);
  free(com_card3);
  free(com_card4);
  free(com_card5);

  return community_string;
}