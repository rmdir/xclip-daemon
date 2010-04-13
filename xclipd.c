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


#include "xclipd.h"

Display	*display;
Window win, root;
size_t buffer_size; 
volatile struct stack *clip_stack;

char * sock_path;

static int stack_init() {
	if((clip_stack = (struct stack *) malloc(sizeof (struct stack))) == NULL)
	       return 1;
	else {	
		clip_stack->top = NULL;
		clip_stack->size = 0;
	}
}

static int push(const char *s, unsigned long l) {
	struct clip_entry *next;
	int newline = 0;
	if ((next = (struct clip_entry *) malloc(sizeof (struct clip_entry))) == NULL)
		return 1;
	else 
		bzero(next, sizeof(struct clip_entry));
	if(s[l-1] != '\n') 
		newline = 1;
	next->len = l+1+newline;
	if ((next->entry = (char *) malloc((next->len)*sizeof(char))) == NULL)
		      return 1;
	else {
		strncpy(next->entry, s,next->len);
		if(newline > 0)
			next->entry[next->len-2] = '\n';
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
	return 0;
}

static void ulisten(void) {
	size_t len, clen;  
	socklen_t fd, cfd;
	char buffer[MAX_CLIP_SIZE + 4];
	char clip_buff[MAX_CLIP_SIZE];
	char cmd[4];
	struct sockaddr_un server, client;
	struct clip_entry *c;

	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}
	bzero(&server, sizeof(struct sockaddr_un));
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, sock_path);

	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if (bind(fd, (struct sockaddr *) &server, len) < 0) {
		perror("bind");
		exit(1);
	}
	listen(fd, 5);
	for(;;){
		clen = sizeof(client);
		if((cfd = accept(fd, (struct sockaddr *) &client, &clen)) < 0){
			perror("accept");
			exit(1);
		}
		else {
			int action=0;
			// Get clip + command size
			int readed = read(cfd, buffer, MAX_CLIP_SIZE + 4 );
			strncpy(cmd,buffer,3);

			if( strcmp("get", cmd ) == 0 ) {
				action = ACTION_GET;
			} else if( strcmp("set",cmd) == 0 ) {
				action = ACTION_SET;
			} else if( strcmp("del",cmd) == 0 ) {
				action = ACTION_DEL;
			} else if( strcmp("siz",cmd) == 0 ) {
				action = ACTION_SIZE;
			}
			switch(action) {
				case ACTION_GET:
					// Client want all off the list
					if(readed < 5) {
						if (clip_stack->size > 0){
							c = clip_stack->top;
							for(;;){
								u_write(cfd, c->entry);
								if(c->next == NULL){
									break;
								}
								else 
									c = c->next;
							}
						}
					}else
					// Client want a specific clip
					if ( readed > 4 ) {
						char * clip_number = (char *) malloc ( (readed - 4) * sizeof(char));
						strncpy( clip_number, buffer + 4, (readed - 4) );
						int clip_nb = atoi(clip_number);
						free(clip_number);
						if(clip_stack->size > clip_nb) {
							c = clip_stack->top;
							int i=0;
							for(i=0; i < clip_nb; i++ ) {
								if(c->next == NULL) {
									break;
								}else{
									c = c->next;
								}
							}
							u_write(cfd, c->entry);
						} else{
							u_write(cfd, "Out of range clip\n");
						}
					}

					break;
				case ACTION_DEL:
					stack_clear();
					break;
				case ACTION_SET:
					strncpy (clip_buff, buffer + 4 , (readed - 4));
					stack_add(clip_buff);
					break;
				case ACTION_SIZE:
					u_write(cfd,"%d",clip_stack->size);
					break;
				default:
					u_write(cfd,"Protocol error\n");
			}
			close(cfd);

		}
	}
}

/* Write to socket */
static int u_write(int socket,const char* format, ...) {
	va_list ap;
	char *p;
	// resolv format
	if ((p = malloc(MAX_CLIP_SIZE * sizeof (char) )) == NULL)
		return 1;
	va_start(ap, format);
	(void) vsnprintf(p, MAX_CLIP_SIZE, format, ap);
	va_end(ap);

	if(write(socket, p, strlen(p))) {
		return 0;
	} else{
		return 1;
	}
}

/* Add clip to stack */
static int stack_add(const char * to_add) {
	push(to_add, strlen(to_add));
}

/* Needs something like a mutex */
static int stack_clear(void) {
	struct clip_entry *supp;
	if(clip_stack == NULL ) {
		return 0;
	}
	if(clip_stack->size == 0) {
		return 0;
	}else {
		if( clip_stack->top != NULL ) {
			supp = clip_stack->top;
			clip_stack->top = supp->next;
			clip_stack->size--;
			free(supp->entry);
			free(supp);
			return stack_clear();
		}else{
			return 0;
		}
	}
}

static void stack_clear_sig(int signum) {
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
		if(push(sel_buf, sel_len) > 0)
			fprintf(stderr, "push problem\n");
		if (sseln == XA_STRING)
			XFree(sel_buf);
		else
			free(sel_buf);
	}
}

static int xlaunch(void) {
	int dummy, event_base, ver_major, ver_minor;
	XEvent	event;
	XFixesSelectionNotifyEvent *sevent;
	if ((display = XOpenDisplay(NULL)) == NULL) {
		fprintf(stderr, "Can't open Display\n");
		return 1;
	}
	root = DefaultRootWindow(display);
	if( !XFixesQueryExtension(display, &event_base, &dummy )){
		fprintf(stderr, "Cannot load XFixes extension\n");
		XCloseDisplay(display);
		return 1;
	}
	if (!XFixesQueryVersion(display, &ver_major, &ver_minor)) {
		fprintf(stderr, "XFixes version not queriable.\n");
		XCloseDisplay(display);
		return 1;
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
	return 0;
}

void clean_exit(int signum) {
	stack_clear();
	free((void *)clip_stack);
	if(sock_path != 0) {
		unlink(sock_path);
	}
	XDestroyWindow(display, win);
	if(display) XCloseDisplay(display);
	exit(0);
}



void usage(void) {
	fprintf(stderr, "Usage:\n"
			"xclipd -n number of entries -s " 
			"/path/to/socket.sock [-d run has a daemon]\n"
	       );
	exit(1);
}


int main(int argc, char **argv) {

	
	int c, dflag = 0;
	pthread_t server;

	signal(SIGINT, clean_exit);
	signal(SIGTERM, clean_exit);
	signal(SIGHUP, stack_clear_sig);



	while ((c = getopt (argc, argv, "ds:n:")) != -1){
		switch (c) {
		case 'd':
			dflag = 1;
			break;
		case 'n':
			buffer_size = atoi(optarg);
			break;
		case 's':
			sock_path = optarg;
			break;
		default :
			usage();
		}
	}
	if(buffer_size == 0 ||
			sock_path == NULL)
		usage();


	stack_init();
	if(dflag > 0){ 
		pid_t pid = fork();
		if(pid > 0){
			printf("%d\n", pid+1);
			exit(0);
		}
		daemon(0,0);
	}
	pthread_create(&server, NULL, (void *)ulisten, NULL);
	if(xlaunch() > 0)
		return 1;
	
	return 0;





}

