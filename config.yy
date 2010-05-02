/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joris.dedieu@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Joris Dedieu
 * ----------------------------------------------------------------------------
 *
 *
 */

%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config.h"



static int number, dflag;
static char *sockpath, *user, *pass;

%}

%union {
	int		val;
	char		*string;
}

%token <string> PATH
%token <val> INTEGER
%token <string> STRING

%token SOCKPATH
%token SIZE 
%token USER
%token PASS
%token DAEMON
%token EOL

%start conf

%%

conf: 
    	|lines
	;

lines:
     	line lines
	|line
	;

line: 	
    		sockline EOL
		| sizeline EOL
		| userline EOL
		| passline EOL
		| daemonline EOL
		| EOL
	;

sockline: 	SOCKPATH PATH { sockpath = strdup($2); }
sizeline:	SIZE INTEGER { number = $2; }
userline: 	USER STRING { user = strdup($2); }
passline:	PASS STRING { pass = strdup($2); }
daemonline:	DAEMON { dflag = 1; }

%%

int yyerror (char *s) {
	fprintf(stderr, "%s\n", s);
	return EXIT_FAILURE;
}

void client_usage(void) {
	fprintf(stderr,
		"Usage : \n\t"
		"clipme [-h] [-s /path/to/socket.sock] [-f /path/to/clipmerc] [command [args]]\n"
		);
	exit(EXIT_FAILURE);
}
void server_usage(void) {
	fprintf(stderr,
		"Usage : \n\t" 
		"clipmed [-hd] [-n size of the history] [-s /path/to/socket.sock] [-f /path/to/clipmerc] [-u user] [-p pass]\n"
		);
	exit(EXIT_FAILURE);
}

struct config *init_config(void){
	struct config *conf;
	char *user; 
	size_t l;
	if((conf = (struct config *) malloc(sizeof(struct config))) == NULL) {
		perror("malloc");
		return NULL;
	}
	bzero(conf,sizeof(struct config));
	conf->number = DEFAULT_STACK_SIZE;
	conf->daemon = 0;
	user = getenv("USER");
	l = sizeof(char) * (strlen(	DEFAULT_SOCK_PATH) 
					+ strlen(user) 
					+ strlen(DEFAULT_SOCK_PREFIX) 
					+ 3);
	if((conf->sockpath = (char *) malloc(l)) == NULL) {
		perror("malloc");
		return NULL;
	}
	(void) snprintf(conf->sockpath, l, "%s/%s.%s",  DEFAULT_SOCK_PATH,
							user,
							DEFAULT_SOCK_PREFIX
			);
	return conf;
}

struct config *parse_args(int argc, char **argv) {
	int c, i, n = 0, dflag = 0;
	char *sockpath = NULL , *confpath = NULL, *command, *buff, *arg, *tmp;
	size_t l;
	struct config *conf;
	while ((c = getopt (argc, argv, "s:n:dh")) != -1) {
		switch(c){
		case 's':
			sockpath = optarg;
                        break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			confpath = optarg;
			break;
		case 'h':
			usage();
			break;
		}
	}
	command = NULL;
	buff = NULL;
	tmp = NULL;
	for(i = optind; i < argc; i++) {
		buff = command;
		arg = argv[i];
		l = sizeof(char) * (strlen(arg) + 1); 
		if(buff == NULL) {
			if((command = (char *) malloc(l)) != NULL) 
				command = strndup(arg,l);
		}
		else {
			l += sizeof(char) * strlen(buff) +1;
			if((tmp = (char *) realloc(command, l)) != NULL) {
				command = tmp;
				(void) sprintf(command, "%s %s", buff, arg);
			}
		}
	}
	if((conf = read_config(confpath)) == NULL) {
		fprintf(stderr, "error : read_config\n");
		if(command) free(command);
		return NULL;
	} 
	if(sockpath != NULL) conf->sockpath = sockpath;
	if(dflag > 0) conf->daemon = 1;
	if(number > 0) conf->number = number;
	if(command != NULL) conf->command = command;
	return conf;
}
void free_config(struct config *config) {
	if(config->sockpath) free(config->sockpath);
	if(config->command) free(config->command);
	if(config->user) free(config->user);
	if(config->pass) free(config->pass);
	free(config);
}	

struct config *read_config(const char *path) {
	char *home,*p; 
	size_t l;
	struct config *conf = NULL;
	struct stat s;

	/* No config file provide */
	if(path == NULL) {
		if((home=getenv("HOME")) == NULL)
			return NULL;

		/* $HOME/.config/clipmerc\0 */
		l = sizeof(char)*(strlen(home) + 18);
		if((p = (char *) malloc(l)) == NULL) {
			perror("malloc");
			return NULL;
		}
		(void) snprintf(p, l, "%s/.config/clipmerc", home);
		if((lstat(p, &s)) == 0) {
			conf = real_read_config(p);
			free(p);
			return conf;
		}

		/* $HOME/.clipmerc\0 */
		l = sizeof(char)*(strlen(home) + 11);
		if((p = (char *) realloc(p,l)) == NULL) {
			perror("realloc");
                        return NULL;
                }
		(void) snprintf(p, l, "%s/.clipmerc", home);
		if((lstat(p, &s)) == 0) {
                        conf = real_read_config(p);
                        free(p);
                        return conf;
		}
	} else  return real_read_config(path);
	return init_config();
}

struct config *real_read_config(const char *path) {
	struct config *conf = NULL;
	FILE *f;
	dflag = 0;
	number = DEFAULT_STACK_SIZE;
	sockpath = NULL;
	if((f = fopen(path, "r")) == NULL) {
		perror("fopen");
		return NULL ;
	}
	yyrestart(f);
	yyparse ();
	fclose(f);
	
	if((conf = init_config()) == NULL) {
		fprintf(stderr, "error init_config\n");
		return NULL;
	}
	if(sockpath != NULL) conf->sockpath = sockpath;
	conf->number = (size_t) number;
	conf->user = user;
	conf->pass = pass;
	conf->daemon = dflag;
	return conf; 

}
		
		
		
