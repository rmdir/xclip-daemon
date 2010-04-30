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

void usage(void) {
	exit(EXIT_FAILURE);
}

struct config *init_config(void){
	struct config *conf;
	char *user, *buf;
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
		free(conf);
		return NULL;
	}
	(void) snprintf(conf->sockpath, l, "%s/%s.%s",  DEFAULT_SOCK_PATH,
							user,
							DEFAULT_SOCK_PREFIX
			);
	free(user);
	return conf;
}

struct config *parse_args(int argc, char **argv) {
	int c, i, n = 0, dflag = 0, bflag = 0;
	char *sockpath = NULL , *confpath = NULL, *command, *buff;
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
	for(i = optind; i < argc; i++) {
		buff = command;
		l = sizeof(char) * (strlen(buff) + strlen(argv[i]) + 2); 
		if(command == NULL)
			command = (char *) malloc(l); 
		else command = (char *) realloc(command, l);
		if(command == NULL) {
			perror("malloc");
			return NULL;
		}
		(void) snprintf(command, l, "%s %s", buff, argv[i]);
	}
	if(buff) free(buff);
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

struct config *read_config(const char *path) {
	char *home,*p; 
	size_t l;
	FILE *f;
	struct config *conf = NULL;

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
		if((f=fopen(p, "rb")) != NULL) {
			fclose(f);
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
		if((f=fopen(p, "rb")) != NULL) {
			fclose(f);
                        conf = real_read_config(p);
                        free(p);
                        return conf;
		}
	}
	return real_read_config(path);
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
	conf->sockpath = sockpath;
	conf->number = (size_t) number;
	conf->user = user;
	conf->pass = pass;
	conf->daemon = dflag;
	return conf; 

}
		
		
		
