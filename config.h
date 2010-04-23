struct config {
	int number;
	char *sockpath;
	char *user;
	char *pass;
};

struct config *read_config(char *path);

