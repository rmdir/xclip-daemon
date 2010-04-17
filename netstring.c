#include <xclipd.h>

int nprintf(int socket, const char* format, ...)
        va_list ap;
        char *p *n;
	size_t size;
        va_start(ap, format);
        (void) sprintf(p,format, ap);
        va_end(ap);
	(void) sprintf(n,"%i:%s", strlen(p), p);
	size=strlen(n)
	n[size]=',';

        if(write(socket, n, size)) {
                return 0;
        } else{
                return 1;
        }
}


