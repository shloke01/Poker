# Poker
Authors:
James Lim and Shloke Meresh

<img width="682" alt="poker" src="https://user-images.githubusercontent.com/75258997/226205748-f02c6f52-ba95-49af-a111-fb655210c203.png">

This is a program that implements a full-functional poker game across a distributed system. It utilizes parallel processing and thread synchronization to take care of the game logic simultaneously between all users. Steps to run the program are outlined below. 

Citations:
    1. Code derived from the work of Charlie Curtsinger
    2. Poker hand evaluator library from https://github.com/HenryRLee/PokerHandEvaluator. Download this library in order for the program to run. The makefile will take care of compiling it correctly. 

Steps to run program:
    1. Change directory to "/project_final/PokerHandEvaluator/cpp" and run "make".
    2. Change directory to "/project_final" and run "make" to compile the program.
    3. Run "./poker_server" in a terminal window. This will start the game server.
    4. On any machine in the network, run the following in a terminal window to join the game as a client
       "./poker_client <enter_username> <enter_computer_name> <enter_server_port>"
       The port number must be taken from what is displayed when the server is run.
    5. As many as MAX_PLAYERS (from poker_server) players can join the server

Steps for each individual player to play the game:
    1. When it is your turn, you can either check, call, fold, or raise, based on the rules of poker
    2. Each player is dealt a hand, and are asked to play when it is their turn. 10$ are taken from their virtual cash as ante.
    3. You cannot check until you have matched the current bet
    4. You cannot call unless someone has raised
    5. You cannot call or raise higher than the amount of virtual cash you have
    6. Process continues until all betting rounds are over, after which the winner is declared and a new hand begins

Brief explanation of betting options:
    Fold - fold your hand, forfeit the amount you have placed in the pot, and wait for the next hand
    Raise - raise the amount of the current bet to a higher amount
    Call - match the highest current bet
    Check - you are fine with the current bet and you want to move to the next betting round

Example:
Player 1 joins the game. 10$ deducted from their vc as ante.
Player 1 raises bet to 20$. This is deducted from their vc and added to the pot.
All other players must either fold or match their bet (call) to continue playing.
Once all players have either checked or called, the next betting round begins.
The community cards are dealt, and it is the first player's turn again. 
The betting rounds progress in order, but we always return to the first (non-folded) player after a betting round is over.
This process is repeated until all betting rounds are completed.
All players' hands are evaluated, and the winner gets the amount of the pot credited to their virtual cash.
A new round begins.

Note: Our program doesn't tolerate players exiting. 
      If at any time a player exits the game, the entire program terminates.
