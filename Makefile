all :
	cc -g xclipd.c xclib.c -o xclipd -lX11 -lXfixes -lXmu -lpthread

clean :
	rm -f *~ xclipd
