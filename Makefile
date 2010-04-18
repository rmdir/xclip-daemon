all :
	cc -O2 clipme.c common.c xclib.c -o clipme -DDAEMON -lX11 -lXfixes -lXmu -lpthread

clean :
	rm -vf *~ clipme

install :
	install -svbm 755 clipme /usr/local/bin/ 

debug : 
	cc -g -Wall clipme.c common.c xclib.c -o clipme -DDAEMON -lX11 -lXfixes -lXmu -lpthread

twitter : 
	cc -g clipme.c xclib.c common.c -o clipme -DDAEMON -DWITH_TWITTER -lcurl -lX11 -lXfixes -lXmu -lpthread

help :
	@echo "one of all clean install debug twitter"
