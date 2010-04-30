CC=cc
LEX=flex
YACC=bison -y -d
CFLAGS=-g -Wall -DWITH_TWITTER
LDFLAGS=-lX11 -lXfixes -lXmu -lpthread -lcurl -lfl
RM=rm -vf
INSTALL=install

all : common server client
common : 
	$(LEX) -o config_lex.c config.l 
	$(YACC) -o config_yy.c config.yy
	$(CC) $(CFLAGS) -c config_lex.c
	$(CC) $(CFLAGS) -c config_yy.c	
	$(CC) $(CFLAGS) -c netstrings.c

server :
	$(CC) $(CFLAGS) -c clipmed.c 
	$(CC) $(CFLAGS) -c xclib.c 
	$(CC) $(CFLAGS) -o clipmed clipmed.o netstrings.o config_yy.o config_lex.o xclib.o $(LDFLAGS) 

client : 
	$(CC) $(CFLAGS) -c clipme.c 
	$(CC) $(CFLAGS) -o clipme netstrings.o config_yy.o config_lex.o $(LDFLAGS) 

clean :
	$(RM) *~ clipme clipmed config_lex.c config_yy.c config_yy.h *.o

install :
	$(INSTALL) -d /usr/local/bin/
	$(INSTALL) -svbm 755 clipme /usr/local/bin/ 



