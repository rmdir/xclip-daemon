all :
	cc -O2 xclipd.c xclib.c -o xclipd -lX11 -lXfixes -lXmu -lpthread

clean :
	rm -vf *~ xclipd

install :
	install -svbm 755 xclipd /usr/local/bin/ 

debug : 
	cc -g -Wall xclipd.c xclib.c -o xclipd -lX11 -lXfixes -lXmu -lpthread

help :
	@echo "one of all clean install debug"
