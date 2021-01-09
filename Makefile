all:
	g++ -Wall -Werror -c common.cc
	g++ -Wall -Werror client.cc common.o -o client
	g++ -Wall -Werror server.cc common.o -lpthread -o server
