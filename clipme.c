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

#include "clipme.h"
#include "config.h"

int main(int argc, char **argv) {
	/* client */
	struct sockaddr_un con;
	struct config *conf;
	char *command;
	char *response = NULL;
	int fd = 0;
	size_t s;
	conf = parse_args(argc, argv);
	command = conf->command; 

	if(command == NULL){
		/* get\n\0 */
		if((command = (char *) malloc(sizeof(char)*5)) == NULL){
			perror("malloc");
			return EXIT_FAILURE;
		}
		s = sizeof(command)+1;
		if(getline(&command, &s, stdin) < 0){
			perror("getline");
			free_config(conf);
			return EXIT_FAILURE;
		}
	}
	bzero(&con,sizeof(con)); 
	con.sun_family = AF_UNIX;
	strcpy(con.sun_path, conf->sockpath);
	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("socket");
		free_config(conf);
		return EXIT_FAILURE;
	}
	if (connect(fd, (struct sockaddr *) &con, sizeof(con)) == 0){
		(void) netprintf(fd,command);
		while((response = netread(fd)) != NULL) {
			(void) printf("%s\n",response);
		}
		close(fd);
	}
	else {
		perror("connect");
		free_config(conf);
		return EXIT_FAILURE;
	}	
	free_config(conf);
	return EXIT_SUCCESS;
}

