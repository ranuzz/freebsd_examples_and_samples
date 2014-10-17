all: client.c server.c
	     gcc -g -o ser server.c
	     gcc -g -o clt client.c
