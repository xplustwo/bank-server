all: clean bank server client

bank:
	touch ./bank
server:
	gcc -pthread -Wall -o server server.c
client:
	gcc -pthread -Wall -o client client.c
cleanmmap:
	rm -f bank 
clean: 
	rm -f client server *.o
