GPGMECONFIG = /usr/local/bin/gpgme-config
CC=gcc
#CFLAGS = -c -Wall `$(GPGMECONFIG) --cflags --libs`
CFLAGS = -c -Wall 

all: duplicator

clean:
	rm duplicator *.o

duplicator: main.o configfile.o smtprelay.o duplicator.o
	$(CC) main.o configfile.o smtprelay.o duplicator.o -o duplicator

main.o: main.c
	$(CC) $(CFLAGS) main.c

configfile.o: configfile.c
	$(CC) $(CFLAGS) configfile.c

smtprelay.o: smtprelay.c
	$(CC) $(CFLAGS) smtprelay.c

duplicator.o:
	$(CC) $(CFLAGS) duplicator.c

