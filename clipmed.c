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


#include "clipme.h"

Display	*display;
Window win, root;
size_t buffer_size; 
volatile struct stack *clip_stack;
static pthread_mutex_t mutex;

char *sock_path;

#ifdef WITH_TWITTER
char *user, *pass;
#endif /* WITH_TWITTER */

static int stack_init() {
	if((clip_stack = (struct stack *) malloc(sizeof (struct stack))) == NULL)
	       	return EXIT_FAILURE;
	else {	
		clip_stack->top = NULL;
		clip_stack->size = 0;
	}
	return EXIT_SUCCESS;
}

int push(const char *s, unsigned long l) {
	struct clip_entry *next;
	int newline = 0;
	pthread_mutex_lock(&mutex);
	if ((next = (struct clip_entry *) malloc(sizeof (struct clip_entry))) == NULL){
		pthread_mutex_unlock(&mutex);
		return EXIT_FAILURE;
	} else 
		bzero(next, sizeof(struct clip_entry));
	next->len = l+1+newline;
	if ((next->entry = (char *) malloc((next->len)*sizeof(char))) == NULL){
		pthread_mutex_unlock(&mutex);
		return EXIT_FAILURE;
	} else {
		strncpy(next->entry, s,next->len);
		next->entry[next->len-1]='\0';
	}
	/* If the same string is selected twice
	 * (maybe a bug in get_selection)
	 */ 
	if(clip_stack->top != NULL){
		if(next->len == clip_stack->top->len){
			if(strcmp(next->entry, clip_stack->top->entry) == 0){
				/*Abort*/
				free(next->entry);
				free(next);
				pthread_mutex_unlock(&mutex);
				return -1;
			}
		}
	}
	/* Suppress the lastone if needed */
	if(clip_stack->size == buffer_size) {
		struct clip_entry *c = clip_stack->top;
		struct clip_entry *prev;
		while(c->next != NULL){
			c = c->next;
			prev = c;
		}
		free(c->entry);
		free(c);
		prev->next = NULL;
		clip_stack->size--;
	}
	next->next = clip_stack->top;
	clip_stack->top = next;
	clip_stack->size++;
	pthread_mutex_unlock(&mutex);
	return EXIT_SUCCESS;
}

struct clip_entry *get_one(int n) {
	struct clip_entry *c;
	int i;
	pthread_mutex_lock(&mutex);
	c = clip_stack->top;
	for(i=0; i < n; i++) {
		if(c->next == NULL){
			pthread_mutex_unlock(&mutex);
			return NULL;
		}	
		c = c->next;
	}
	pthread_mutex_unlock(&mutex);
	return c;
}

void ulisten(void) {
	size_t len, clen;  
	socklen_t fd, cfd;
	char *buffer, *cmd, *args = NULL;
	struct sockaddr_un server, client;
	struct clip_entry *c;
	int i;
#ifdef WITH_TWITTER
	CURL *curl;
	CURLcode res;
	char *login, *message;
#endif /* WITH_TWITTER */


	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	bzero(&server, sizeof(struct sockaddr_un));
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, sock_path);

	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if (bind(fd, (struct sockaddr *) &server, len) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	listen(fd, 5);
	for(;;){
		clen = sizeof(client);
		if((cfd = accept(fd, (struct sockaddr *) &client, &clen)) < 0){
			perror("accept");
			exit(EXIT_FAILURE);
		}
		else {
			int action=0;
			/* get clip command */
			if((buffer=netread(cfd)) == NULL){
				close(cfd);
				continue;
			}
			if((cmd = (char *) malloc(sizeof(char)*4)) == NULL){
				perror("malloc");
				continue;
			}
			(void) strncpy(cmd, buffer, (size_t) 3);
			cmd[3]='\0';
			if(strlen(buffer) > 3){
				/* 4 cmd + ' ' */
				if((args = (char *) malloc(sizeof(char) * (strlen(buffer) -4))) == NULL){
					perror("malloc");
					continue;
				}
				args = strndup(buffer + 4, strlen(buffer) -4);
			}
			free(buffer);
			if( strcmp("get", cmd ) == 0 ) {
				action = ACTION_GET;
			} else if( strcmp("set",cmd) == 0 ) {
				action = ACTION_SET;
			} else if( strcmp("del",cmd) == 0 ) {
				action = ACTION_DEL;
			} else if( strcmp("siz",cmd) == 0 ) {
				action = ACTION_SIZE;
#ifdef WITH_TWITTER
			} else if(strcmp("twt", cmd) == 0 ) {
				action = ACTION_TWIT;
#endif /* WITH_TWITTER */
			}
			free(cmd);
			switch(action) {
				case ACTION_GET:
					/* Client wants all off the list */
					if(args == NULL) {
						for(i=0; i < clip_stack->size; i++){
							if((c=get_one(i)) != NULL)
								netprintf(cfd, c->entry);
							else
								break;
						}
					}
					/* Client want a specific clip */
					else {
						/* check if args is a number */
						short valid_arg = 1;
						for(i = 0; i < strlen(args); i++){
							if(!isdigit(args[i]) ){
								netprintf(cfd,"Protocol error");
								valid_arg = 0;
							}
						}
						if(!valid_arg)
							break;
						if( atoi(args) <= clip_stack->size && (c=get_one(atoi(args))) != NULL)
							netprintf(cfd,c->entry);
						else{
							netprintf(cfd, "Out of range clip");
							break;
						}
					}
					break;
				case ACTION_DEL:
					stack_clear();
					break;
				case ACTION_SET:
					stack_add(args);
					break;
				case ACTION_SIZE:
					netprintf(cfd,"%d",clip_stack->size);
					break;
#ifdef WITH_TWITTER
				case ACTION_TWIT:
					if(user == NULL || pass == NULL){
						fprintf(stderr, "-u and -p options must be set for twitter use\n");
						break;
					}
					curl = curl_easy_init();
					if((login = (char *) malloc(sizeof(char) *(strlen(user) + strlen(pass) + 2))) == NULL){
						perror("malloc");
						break;
					}
					(void) sprintf(login,"%s:%s", user,pass);
					curl_easy_setopt(curl, CURLOPT_URL, TWIT_URL);
					curl_easy_setopt(curl, CURLOPT_USERPWD, login);
                 			curl_easy_setopt(curl, CURLOPT_POST, 1);
					for(i = 0; i < strlen(args); i++){
						if(!isdigit(args[i])){
							netprintf(cfd,"Protocol error");
						}
					}
					if((c=get_one(atoi(args))) == NULL)
							netprintf(cfd, "Out of range clip");
					else {
						if((message = (char *) malloc(sizeof(char) * (10 + strlen(c->entry)))) == NULL){
							perror("malloc");
							break;
						}
						(void) sprintf(message, "status=\"%s\"", c->entry);
						curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message);
						res = curl_easy_perform(curl);
                 				curl_easy_cleanup(curl);
						if(res == 0) 
							netprintf(cfd, "Entry : \"%s\" was posted", c->entry);
						else 
							netprintf(cfd, "Error(%d) while posting \"%s\"", res, c->entry);
						
					}
					break;
#endif
				default:
					netprintf(cfd,"Protocol error");
			}
			close(cfd);
			if(args != NULL) free(args);
		}
	}
}

/* Add clip to stack */
void stack_add(const char * to_add) {
	push(to_add, strlen(to_add));
}

int stack_clear(void) {
	pthread_mutex_lock(&mutex);
	int res = _stack_clear();
	pthread_mutex_unlock(&mutex);
	return res;
}

int _stack_clear(void) {
	struct clip_entry *supp;
	if(clip_stack == NULL ) {
		return EXIT_SUCCESS;
	}
	if(clip_stack->size == 0) {
		return EXIT_SUCCESS;
	}else {
		if( clip_stack->top != NULL ) {
			supp = clip_stack->top;
			clip_stack->top = supp->next;
			clip_stack->size--;
			if(supp->entry) free(supp->entry);
			if(supp) free(supp);
			return _stack_clear();
		}else{
			return EXIT_SUCCESS;
		}
	}
}

void stack_clear_sig(int signum) {
	stack_clear();
}


/* Directly from xclip 
 * Clean up needed
 */
static void get_selection(void) {
	unsigned char *sel_buf;	/* buffer for selection data */
	unsigned long sel_len = 0;	/* length of sel_buf */
	XEvent evt;			/* X Event Structures */
	unsigned int context = XCLIB_XCOUT_NONE;

	Atom sseln = XA_PRIMARY; 
	Atom target = XA_UTF8_STRING(display);

	xcout(display, win, evt, sseln, target, &sel_buf, &sel_len, &context);
	if(context != XCLIB_XCOUT_NONE){
		XNextEvent(display, &evt);
		xcout(display, win, evt, sseln, target, &sel_buf, &sel_len, &context);
	}

	if(sel_len) {
		if(push((char *)sel_buf, sel_len) > 0)
			(void) fprintf(stderr, "push problem\n");
		if (sseln == XA_STRING)
			XFree(sel_buf);
		else
			if(sel_buf) free(sel_buf);
	}
}

static int xlaunch(void) {
	int dummy, event_base, ver_major, ver_minor;
	XEvent	event;
	XFixesSelectionNotifyEvent *sevent;
	if ((display = XOpenDisplay(NULL)) == NULL) {
		(void) fprintf(stderr, "Can't open Display\n");
		return EXIT_FAILURE;
	}
	root = DefaultRootWindow(display);
	if( !XFixesQueryExtension(display, &event_base, &dummy )){
		(void) fprintf(stderr, "Cannot load XFixes extension\n");
		XCloseDisplay(display);
		return EXIT_FAILURE;
	}
	if (!XFixesQueryVersion(display, &ver_major, &ver_minor)) {
		(void) fprintf(stderr, "XFixes version not queriable.\n");
		XCloseDisplay(display);
		return EXIT_FAILURE;
	}
	win = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);
	XFixesSelectSelectionInput(display, win, XA_PRIMARY, XFixesSetSelectionOwnerNotifyMask);
	for(;;){
		XNextEvent(display, &event);
		if (event.type == XFixesSelectionNotify + event_base)
		{
			sevent = (XFixesSelectionNotifyEvent *) & event;
			if (sevent->subtype == XFixesSetSelectionOwnerNotify)
			{
				get_selection();
			}
		}
	}
	return EXIT_SUCCESS;
}

static void clean_exit(int signum) {
	stack_clear();
	if(clip_stack) free((void *)clip_stack);
	if(sock_path != 0) {
		unlink(sock_path);
	}
	XDestroyWindow(display, win);
	if(display) XCloseDisplay(display);
	exit(0);
}



static void usage(void) {
	(void) fprintf(stderr, 	"\nDaemon usage :\n"
				"\tclipmed -n number of entries -s " 
				"/path/to/socket.sock -d\n"
	       );
#ifdef WITH_TWITTER
	(void) fprintf(stderr, 	"\tIf you want to use twitter feature add a "
				"-u username and -p password\n"
		      );
#endif /* WITH_TWITTER */
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {
	int c, dflag = 0;
	pthread_t server;

#ifndef WITH_TWITTER
	while ((c = getopt (argc, argv, "ds:n:")) != -1){
#else
	while ((c = getopt (argc, argv, "ds:n:u:p:")) != -1){
#endif /* WITH_TWITTER */
		switch (c) {
		case 'd':
			dflag = 1;
			break;
		case 'n':
			buffer_size = atoi(optarg);
			if(buffer_size > MAX_STACK_SIZE){
				fprintf(stderr, "Buffer Size %d "
					      	"is greater than %d. "
				      		"Please increase MAX_STACK_SIZE in clipme.h\n",
				  		buffer_size, MAX_STACK_SIZE);
				buffer_size = MAX_STACK_SIZE;
			}		
			break;
		case 's':
			sock_path = optarg;
			break;
#ifdef WITH_TWITTER
		case 'u':
                	user = optarg;
                        break;
                case 'p':
                        pass = optarg;
                        break;
#endif /* WITH_TWITTER */
		default :
			usage();
		}
	}
	if(sock_path == NULL || buffer_size <= 0)
		usage();
	/* daemon */
	stack_init();
	if(dflag > 0){
		pid_t pid = fork();
		if(pid > 0){
			printf("%d\n", pid+1);
			exit(0);
		}
		daemon(0,0); 
	}

	signal(SIGINT, clean_exit);
	signal(SIGTERM, clean_exit);
	signal(SIGHUP, stack_clear_sig);

	pthread_mutex_init(&mutex, NULL);
	pthread_create(&server, NULL, (void *)ulisten, NULL);
	if(xlaunch() > 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

