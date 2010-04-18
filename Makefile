all :
	cc -O2 clipmed.c common.c xclib.c -o clipmed -DDAEMON -lX11 -lXfixes -lXmu -lpthread

client :
	cc -O2 clipme.c common.c -o clipme

clean :
	rm -vf *~ clipme clipmed

install :
	install -svbm 755 clipme /usr/local/bin/ 

debug : 
	cc -g -Wall clipmed.c common.c xclib.c -o clipmed -DDAEMON -lX11 -lXfixes -lXmu -lpthread

twitter : 
	cc -g clipmed.c xclib.c common.c -o clipmed -DDAEMON -DWITH_TWITTER -lcurl -lX11 -lXfixes -lXmu -lpthread

help :
	@echo "one of all clean install debug twitter"
