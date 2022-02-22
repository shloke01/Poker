#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <phevaluator/phevaluator.h>
#include <phevaluator/rank.h>

// Taken from Charlie Curtsinger's worm lab
// Returns time in miliseconds
size_t time_ms();

// Struct for a card
typedef struct card {
  int val;
  int suit;
  int total;
} card_t;

// Struct for a deck
typedef struct deck {
  int card[52];
} deck_t;

// Struct for a hand of 2 cards
typedef struct hand {
  card_t card[2];
} hand_t;

// Struct for community cards
typedef struct commmunity {
  card_t card[5];
} community_t;

//*********
// Globals for poker game

// Deck of 52 cards
deck_t deck;

// Struct of community cards
community_t community;

//******************


// Take card and return rank and face as string
char* card_to_string(card_t card);

// Initialize deck to full deck of 52 cards (no jokers)
void deck_init();

// Initialize community cards
void community_init();

// Remove a given card from the deck
void remove_card_from_deck(int card);

// Generate a random card from the deck
card_t rand_card();

// Print the deck 
void print_deck();

// Deal the community cards
community_t deal_community();

// Deal a hand of two cards
hand_t deal_hand();

// Get string of flop, turn, or river
char* flop_turn_river(int betting_round);