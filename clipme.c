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

void usage(void) {
	(void) fprintf(stderr, 
			"\nUsage :\n\tclipme -s /path/to/socket < cmd\n\n");
	exit(EXIT_FAILURE);
}	

int main(int argc, char **argv) {
	int c, dflag = 0, bflag = 0;
	pthread_t server;
	/* client */
	char *command = NULL, *response = NULL;
	struct sockaddr_un con;
	int fd = 0;
	size_t s;


	while ((c = getopt (argc, argv, "s:")) != -1){
		switch (c) {
		case 's':
			sock_path = optarg;
			break;
		default :
			usage();
		}
	}
	if(sock_path == NULL)
		usage();
	if(command == NULL){
		/* get\n\0 */
		if((command = (char *) malloc(sizeof(char)*5)) == NULL){
			perror("malloc");
			return EXIT_FAILURE;
		}
		s = sizeof(command)+1;
		if(getline(&command, &s, stdin) < 0){
			perror("getline");
			return EXIT_FAILURE;
		}
	}
	bzero(&con,sizeof(con)); 
	con.sun_family = AF_UNIX;
	strcpy(con.sun_path, sock_path);
	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("socket");
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
		return EXIT_FAILURE;
	}	
	return EXIT_SUCCESS;
}

