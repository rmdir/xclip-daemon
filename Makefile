all :
	cc -O2 clipmed.c netstrings.c xclib.c -o clipmed -DDAEMON -lX11 -lXfixes -lXmu -lpthread

client :
	cc -g clipme.c netstrings.c -o clipme

clean :
	rm -vf *~ clipme clipmed config_lex.c config_yy.c config_yy.h

install :
	install -svbm 755 clipme /usr/local/bin/ 

debug : 
	cc -g -Wall clipmed.c netstrings.c xclib.c -o clipmed -DDAEMON -lX11 -lXfixes -lXmu -lpthread

twitter : 
	cc -g clipmed.c xclib.c netstrings.c -o clipmed -DDAEMON -DWITH_TWITTER -lcurl -lX11 -lXfixes -lXmu -lpthread

help :
	@echo "one of all clean install debug twitter"

config :
	flex -o config_lex.c config.l 
	bison -y -d -o config_yy.c config.yy
	cc -g -c config_lex.c
	cc -g -c config_yy.c	
