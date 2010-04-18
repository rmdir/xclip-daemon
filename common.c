#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>


struct config {
	int number;
	char *sock_path;
#ifdef WITH_TWITTER
	char *user;
	char *pass;
#endif /* WITH_TWITTER */
};

int netprintf(int socket, const char* format, ...);
char *netread(int socket);
struct config *parse_conf(char *path); 

#define CONF1 "~/.config/clipmerc"
#define CONF2 "~/.clipmerc"
#define MAX_LINE_LENGHT 255

inline void free_conf(struct config *conf) {
	if(conf->sock_path != NULL) free(conf->sock_path);
#ifdef WITH_TWITTER
	if(conf->user != NULL) free(conf->user);
	if(conf->pass !=NULL) free(conf->pass);
#endif /* WITH_TWITTER */
	free(conf);
}

struct config *parse_conf(char *path) {
	struct stat *s;
	struct config *conf;
	FILE *f;
	char *c, *p, *token, *value;
	int i,j;
	/* default in ~/.config/clipmerc or ~/.clipmerc */
	if(path == NULL){
		if(lstat(CONF1,s) == 0)
			strcpy(path,CONF1);
		else if(lstat(CONF2,s) == 0)
			strcpy(path,CONF2);
		else return NULL;
	}	
	if((f = fopen(path, "r")) == NULL){
		perror("fopen");
		return NULL;
	}
	if((conf = (struct config *) malloc(sizeof(struct config))) == NULL){
		perror("malloc");
		return NULL;
	}
	bzero(conf, sizeof(struct config));
	while(fgets(c, MAX_LINE_LENGHT, f) != NULL){
		if(c[strlen(c) -1] != '\n'){
			fprintf(stderr, "Line too long in %s\n", path);
			fclose(f);
			return NULL;
		}
		if(c[0] == '#' || c[0] == ' ' || c[0] == '\n'){
			continue;
		}
		for(i=0; i < strlen(c); i++){ 
			if(c[i] == ' '|| c[i] == '\t')
				break;
			token[i]=c[i];
		}
		token[i++]='\0';
		/* eliminate many spaces or tabs */
		for(i; i < strlen(c); i++){
			if(c[i] == ' ' || c[i] == '\t')
				continue;
			else break;
		}
		for(i,j=0; i < strlen(c); i++,j++){
			if(c[i] == '\n')
				break;
			value[j]=c[i];
		}
		if(strcmp(token,"number") == 0){
			conf->number=atoi(value);
		} 
		else if(strcmp(token,"sock_path") == 0){
			if((conf->sock_path = (char *) malloc(sizeof(char)*(strlen(token)+1))) == NULL){
				perror("malloc");
				free_conf(conf);
				fclose(f);
				return NULL;
			}
			(void) strcpy(conf->sock_path,token);
			conf->sock_path[strlen(token)] = '\0';
		}
#ifdef WITH_TWITTER
		else if(strcmp(token,"user") == 0){
			if((conf->user = (char *) malloc(sizeof(char)*(strlen(token)+1))) == NULL){
				perror("malloc");
				free_conf(conf);
				fclose(f);
				return NULL;
			}
			(void) strcpy(conf->user,token);
			conf->user[strlen(token)] = '\0';
		}
		else if(strcmp(token,"sock_path") == 0){
			if((conf->pass = (char *) malloc(sizeof(char)*(strlen(token)+1))) == NULL){
				perror("malloc");
				free_conf(conf);
				fclose(f);
				return NULL;
			}
			(void) strcpy(conf->sock_path,token);
			conf->pass[strlen(token)] = '\0';
		}
#endif /* WITH_TWITTER */
	} /* fgets */
	fclose(f);
	return conf;
}

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




