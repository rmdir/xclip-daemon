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

#ifndef CONFIG_H
#define CONFIG_H
#define DEFAULT_STACK_SIZE 20
#define DEFAULT_SOCK_PATH "/var/tmp"
#define DEFAULT_SOCK_PREFIX "cmsock"

struct config {
	size_t number;
	int daemon;
	char *sockpath;
	char *command;
	char *user;
	char *pass;
};

void usage(void);
struct config *init_config(void);
struct config *parse_args(int argc, char **argv);
struct config *real_read_config(const char *path);
struct config *read_config(const char *path);

#endif /* CONFIG_H */
