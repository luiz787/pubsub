all:
	g++ -g -Wall -Werror -c common.cc
	g++ -g -Wall -Werror client.cc common.o -o client
	g++ -g -Wall -Werror server.cc common.o -lpthread -o server
