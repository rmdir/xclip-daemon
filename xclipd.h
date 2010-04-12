/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joris.dedieu@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Joris Dedieu
 * ----------------------------------------------------------------------------
 */

#define ACTION_GET 1
#define ACTION_DEL 2
#define ACTION_SET 3

#define MAX_CLIP_SIZE 1024

Display	*display;
Window win, root;
size_t buffer_size; 
int running;

struct clip_entry {
	char *entry;
	size_t len;
	struct clip_entry *next;
};

struct stack {
	struct clip_entry *top;
	size_t size;
};



static void usage(void);
static void clean_exit(int signum);
static int xlaunch(void);
static void get_selection(void);
static void stack_clear_sig(int signum);
static int stack_clear(void);
static int stack_add(const char * to_add);
static void ulisten(void);
static int push(const char *s, unsigned long l); 
static int stack_init(void); 

