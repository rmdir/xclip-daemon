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

#ifndef CLIPME_H
#define CLIPME_H

//unix
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

//x11
#include <X11/Xlib.h>
#include <X11/Xatom.h> 
#include <X11/Xmu/Atoms.h>
#include <X11/extensions/Xfixes.h> 

//libcurl
#ifdef WITH_TWITTER
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#endif /* WITH_TWITTER */

#include "xclip.h"

/* maybe an enum here ? */
#define ACTION_GET 1
#define ACTION_DEL 2
#define ACTION_SET 3
#define ACTION_SIZE 4
#ifdef WITH_TWITTER
#define ACTION_TWIT 5
#define TWIT_URL "http://twitter.com/statuses/update.xml"
#endif /* WITH_TWITTER */

#define MAX_STACK_SIZE 1024

/*Display	*display;
Window win, root;
size_t buffer_size; 
volatile struct stack *clip_stack;
static pthread_mutex_t mutex;
*/
char *sock_path; 

#ifdef WITH_TWITTER
char *user, *pass;
#endif /* WITH_TWITTER */

struct clip_entry {
	char *entry;
	size_t len;
	struct clip_entry *next;
};

struct stack {
	struct clip_entry *top;
	size_t size;
};



extern int netprintf(int socket,const char *format, ...);
extern char *netread(int socket);
extern void client_usage(void);
extern void server_usage(void);
extern struct config *parse_args(int argc, char **argv);
int stack_init(void); 
void clean_exit(int signum);
void get_selection(void);
void usage(void);
int xlaunch(void);
int stack_clear(void);
int _stack_clear(void);
void stack_clear_sig(int signum);
void stack_add(const char * to_add);
void ulisten(void);
int push(const char *s, unsigned long l); 
struct clip_entry *get_one(int n); 

#endif /* CLIPME_H */
