#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h> 
#include <X11/Xmu/Atoms.h>
#include <X11/extensions/Xfixes.h> 

#include "xclib.h"

Display	*display;
Window win, root;
size_t buffer_size; 

struct clip_entry {
	char *entry;
	size_t len;
	struct clip_entry *next;
};

struct stack {
	struct clip_entry *top;
	size_t size;
};



static struct stack *clip_stack;

static int stack_init() {
	if((clip_stack = (struct stack *) malloc(sizeof (struct stack))) == NULL)
	       return 1;
	else {	
		clip_stack->top = NULL;
		clip_stack->size = 0;
	}
}

static int push(char *s, size_t l) {
	struct clip_entry *next;
	if ((next = (struct clip_entry *) malloc(sizeof (struct clip_entry))) == NULL)
		return 1;
	else 
		bzero(next, sizeof(struct clip_entry));
	next->len = l+1;
	if ((next->entry = (char *) malloc((next->len)*sizeof(char))) == NULL)
		      return 1;
	else {
		strncpy(next->entry, s,next->len);
		next->entry[l]='\0';
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
				return 1;
			}
		}
	}
	/* Suppress the lastone if needed */
	if(clip_stack->size == buffer_size) {
		struct clip_entry *c = clip_stack->top;
		while(c->next != NULL){
			c = c->next;
		}
		free(c->entry);
		free(c);
		clip_stack->size--;
	}
	next->next = clip_stack->top;
	clip_stack->top = next;
	clip_stack->size++;
	printf("%s\n",clip_stack->top->entry);
	return 0;
}

static void ulisten(const char *path) {
	int fd, len, cfd, clen;  
	char buffer[4];
	struct sockaddr_un server, client;
	struct clip_entry *c;

	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("socket");
		return;
	}
	bzero(&server, sizeof(struct sockaddr_un));
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, path);

	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if (bind(fd, (struct sockaddr *) &server, len) < 0) {
		perror("bind");
		return;
	}
	listen(fd, 5);
	for(;;){
		clen = sizeof(client);
		if((cfd = accept(fd, (struct sockaddr *) &client, &clen)) < 0)
			return;
		else {
			read(cfd, buffer, 3);
			if(strcmp("get", buffer) == 0){
				if (clip_stack->size > 0){
					c = clip_stack->top;
					while(c->next != NULL){
						write(cfd, c->entry, c->len);
						write(cfd, '\n', 1);
						printf("arrg : %s", c->entry); 
						c = c->next;
					}
				}
			}
			else if(strcmp("del", buffer) == 0){
				clear();
			}
			else write(cfd,"Protocol error",15);
			close(cfd);
		}
	}
}


int clear(void) {
	struct clip_entry *supp;
	if(clip_stack->size == 0)
		return 0;
	else {
		supp = clip_stack->top;
		clip_stack->top = supp->next;
		clip_stack->size--;
		free(supp->entry);
		free(supp);
		clear();
	}
}


/* Directly from xclip 
 * Clean up need
 */
static void get_selection() {
	unsigned char *sel_buf;	/* buffer for selection data */
	unsigned long sel_len = 0;	/* length of sel_buf */
	XEvent evt;			/* X Event Structures */
	unsigned int context = XCLIB_XCOUT_NONE;

	Atom sseln = XA_PRIMARY; 
	Atom target = XA_UTF8_STRING(display);

	for(;;) {
		/* only get an event if xcout() is doing something */
		if(context != XCLIB_XCOUT_NONE)
			XNextEvent(display, &evt);
		/* fetch the selection, or part of it */
		xcout(display, win, evt, sseln, target, &sel_buf, &sel_len, &context);

		/* fallback is needed. set XA_STRING to target and restart the loop. */
		if(context == XCLIB_XCOUT_FALLBACK) {
			context = XCLIB_XCOUT_NONE;
			target = XA_STRING;
			continue;
		}

		/* only continue if xcout() is doing something */
		if(context == XCLIB_XCOUT_NONE)
			break;
	}
	if(sel_len) {
		/*char *dest = malloc(sizeof(char)*sel_len+1);
		strncpy(dest, sel_buf,sel_len);
		dest[sel_len]='\0';
		printf("%s\n",dest);*/
		push(sel_buf, sel_len);
		if (sseln == XA_STRING)
			XFree(sel_buf);
		else
			free(sel_buf);
		//free(dest);
	}
}

int main(void) {

	int screen, dummy, event_base, ver_major, ver_minor;
	XEvent	event;
	XFixesSelectionNotifyEvent *sevent;
	Atom atom;
	pthread_t server;

	buffer_size = 50;
	stack_init();



	if ((display = XOpenDisplay(NULL)) == NULL) {
		fprintf(stderr, "Can't open Display\n");
		XCloseDisplay(display);
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
	pthread_create(&server, NULL, ulisten, "/home/joris/clip.sock");
	for(;;) {
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
	/* todo deal with sigint to exit correctly
	 * and execute the code below */
	clear();
	free(clip_stack);
	XDestroyWindow(display, win);
	XCloseDisplay(display);
	return 0;
}

