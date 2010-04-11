/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joris.dedieu@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Joris Dedieu
 * ----------------------------------------------------------------------------
 */

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



static struct stack *clip_stack;
char * sock_path;

void usage(void);
void clean_exit(int signum);
static int xlaunch(void);
static void get_selection(void);
void stack_clear_sig(int signum);
int stack_clear(void);
static void ulisten(void);
static int push(char *s, size_t l);
static int stack_init(void); 

