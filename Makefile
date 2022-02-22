CC := clang
CFLAGS := -g -IPokerHandEvaluator/cpp/include -LPokerHandEvaluator/cpp

all: poker_server poker_client

clean:
	rm -f poker_server poker_client 

poker_client: poker_client.c socket.h
	$(CC) $(CFLAGS) -o poker_client poker_client.c -lpthread 

poker_server: poker_server.c poker_functions.c socket.h poker_functions.h
	$(CC) $(CFLAGS) -o poker_server poker_server.c poker_functions.c -lpthread -lpheval -lform -fsanitize=address

