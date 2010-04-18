all :
	cc -O2 clipme.c xclib.c -o clipme -lX11 -lXfixes -lXmu -lpthread

clean :
	rm -vf *~ clipme

install :
	install -svbm 755 clipme /usr/local/bin/ 

debug : 
	cc -g -Wall clipme.c xclib.c -o clipme -lX11 -lXfixes -lXmu -lpthread

twitter : 
	cc -g clipme.c xclib.c -o clipme -DWITH_TWITTER -lcurl -lX11 -lXfixes -lXmu -lpthread

help :
	@echo "one of all clean install debug twitter"
