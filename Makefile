all:
	g++ -g -Iinclude -Wall -Werror -c src/message.cc -o out/message.o
	g++ -g -Iinclude -Wall -Werror -c src/console.cc -o out/console.o
	g++ -g -Iinclude -Wall -Werror -c src/messagebroker.cc -o out/messagebroker.o
	g++ -g -Iinclude -Wall -Werror -c src/common.cc -o out/common.o
	g++ -g -Iinclude -Wall -Werror src/client.cc out/common.o out/console.o -lpthread -o cliente
	g++ -g -Iinclude -Wall -Werror src/server.cc out/message.o out/messagebroker.o out/common.o -lpthread -o servidor
clean:
	rm out/*.o
	rm cliente servidor
