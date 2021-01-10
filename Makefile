all:
	g++ -g -Wall -Werror -c message.cc
	g++ -g -Wall -Werror -c messagebroker.cc
	g++ -g -Wall -Werror -c common.cc
	g++ -g -Wall -Werror client.cc common.o -lpthread -o client
	g++ -g -Wall -Werror server.cc message.o messagebroker.o common.o -lpthread -o server
