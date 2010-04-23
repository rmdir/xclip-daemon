%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"



static int number;
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
%token EOL

%%

conf: 	lines
	;

lines:		line lines
     		| line
	;

line: 	sockline EOL
		| sizeline EOL
		| userline EOL
		| passline EOL
		| EOL
	;

sockline: 	SOCKPATH PATH { sockpath = strdup($2); }
sizeline:	SIZE INTEGER { number = $2; }
userline: 	USER STRING { user = strdup($2); }
passline:	PASS STRING { pass = strdup($2); }

%%

int yyerror (char *s) {
	fprintf(stderr, "%s\n", s);
	return EXIT_FAILURE;
}


struct config *read_config(char *path) {
	struct config *conf = NULL;
	FILE *f;
	number = 0;
	sockpath = NULL;
	if((f = fopen(path, "r")) == NULL) {
		perror("fopen");
		return NULL ;
	}
	yyrestart(f);
	yyparse ();
	fclose(f);
	
	if((conf = malloc(sizeof(struct config))) == NULL) {
		perror("malloc");
		return NULL;
	}
	conf->sockpath = sockpath;
	conf->number = number;
	conf->user = user;
	conf->pass = pass;
	return conf; 

}
