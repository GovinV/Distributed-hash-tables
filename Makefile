CC = gcc
LFLAGS = -g -W -Wall -Werror -o
CFLAGS = -c -g -W -Wall -Werror 

SRC = $(wildcard *.c)

PROGS = server client

all : $(PROGS)

server : server.c stockage_serveur.o  messages.o
	@ $(CC) $(LFLAGS) server server.c stockage_serveur.o  messages.o $(LDFLAGS)

client : client.c messages.o
	@ $(CC) $(LFLAGS) client client.c messages.o  $(LDFLAGS)

messages.o : messages.c messages.h
	@ $(CC) $(CFLAGS) messages.c -o messages.o

stockage_serveur.o : stockage_serveur.c stockage_serveur.h
	@ $(CC) $(CFLAGS) stockage_serveur.c -o stockage_serveur.o

clean:
	@ rm -f *.o
	@ rm -f $(PROGS)

