CC=cc
LEX=flex
YACC=bison -y -d
CFLAGS=-O2 -DWITH_TWITTER
LDFLAGS=-lX11 -lXfixes -lXmu -lpthread -lcurl -lfl
RM=rm -vf
INSTALL=install
GZIP=gzip -f -9
POD2MAN=pod2man -c ""
MANDIR=/usr/local/share/man

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
	$(CC) $(CFLAGS) -o clipme netstrings.o config_yy.o config_lex.o clipme.o $(LDFLAGS) 

clean :
	$(RM) *~ clipme clipmed config_lex.c config_yy.c config_yy.h *.o

install : man
	$(INSTALL) -d /usr/local/bin
	$(INSTALL) -svbm 755 clipme /usr/local/bin 
	$(INSTALL) -svbm 755 clipmed /usr/local/bin 
	$(INSTALL) -d /usr/local/share/examples/clipme
	$(INSTALL) clipmerc.sample /usr/local/share/examples/clipme

man :
	$(INSTALL) clipme.1 $(MANDIR)/man1
	$(GZIP) $(MANDIR)/man1/clipme.1
	$(INSTALL) clipmed.1 $(MANDIR)/man1
	$(GZIP) $(MANDIR)/man1/clipmed.1
	$(INSTALL) clipmerc.5 $(MANDIR)/man5
	$(GZIP) $(MANDIR)/man5/clipmerc.5

mancompile :
	$(POD2MAN) clipme.pod clipme.1
	$(POD2MAN) clipmed.pod clipmed.1
	$(POD2MAN) clipmerc.pod clipmerc.5




