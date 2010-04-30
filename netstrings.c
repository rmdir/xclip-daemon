/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joris.dedieu@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Joris Dedieu
 * ----------------------------------------------------------------------------
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

int netprintf(int socket, const char* format, ...);
char *netread(int socket);


/* Write to socket */
int netprintf(int socket, const char* format, ...){
        va_list ap;
        char *resolved, *netstring, *number;
        size_t len;
	/* resolv the format */
        va_start(ap, format);
	if((resolved=(char *) malloc(sizeof(char))) == NULL){
		perror("malloc");
		return EXIT_FAILURE;
	}
        len=vsnprintf(resolved,1,format, ap);
	if(len >=1){
		len++;
		if((resolved=(char *) realloc(resolved,sizeof(char)*len)) == NULL){
			perror("realloc");
			return EXIT_FAILURE;
		}
        	(void) vsnprintf(resolved,len,format, ap);
	}
        va_end(ap);
	/* compose the netstring */
	if((number=(char *) malloc(sizeof(char))) == NULL){
		perror("malloc");
		return EXIT_FAILURE;
	}
	len=snprintf(number,1,"%d",strlen(resolved));
	if(len >=1){
		len++;
		if((number=(char *) realloc(number,sizeof(char)*len)) == NULL){
			perror("realloc");
			return EXIT_FAILURE;
		}
		(void) snprintf(number,len,"%d",strlen(resolved));
	}
	len += sizeof(char) * strlen(resolved);
	len += sizeof(char) * 2; // ':' + '\0'
	if((netstring=(char *) malloc(len)) == NULL){
		perror("malloc");
		return EXIT_FAILURE;
	}

        len=snprintf(netstring,len,"%s:%s", number, resolved);
	if(resolved) free(resolved);
	/* change \0 to , */
        netstring[len]=',';

	/* Write to socket */
        if(write(socket, netstring, len+1)) {
		/* free need a limit */
		netstring[len]='\0';
		if(netstring) free(netstring);
                return EXIT_SUCCESS;
        } else{
		netstring[len]='\0';
		if(netstring) free(netstring);
                return EXIT_FAILURE;
        }
}

char *netread(int socket){
	char *buffer, c[2];
	size_t len=0;
	if((buffer = (char *) malloc(sizeof(char))) == NULL){
		perror("malloc");
		return NULL;
	}
	for(;;){
		read(socket,&c, 1);
		c[1]='\0';
		if(c[0] == ':')
		       break;
		if(isdigit(c[0])){
			len = 10 * len + atoi(c);	
		}
		else {
#ifdef DAEMON
			fprintf(stderr, "Not a netstring\n");
#endif /* DAEMON */
			return NULL;
		}
	}
	if(len > 0){
		if((buffer = (char *) malloc(sizeof(char) * (len +1))) == NULL){
			perror("malloc");
			return NULL;
		}
		read(socket, buffer, len + 1);
		if(buffer[len] != ','){
#ifdef DAEMON
			fprintf(stderr, "Not a netstring\n");
#endif /* DAEMON */
			free(buffer);
			return NULL;
		}
		buffer[len] = '\0';
		return buffer;
	}
#ifdef DAEMON
	fprintf(stderr, "Not a netstring\n");
#endif /* DAEMON */
	return NULL;
}




