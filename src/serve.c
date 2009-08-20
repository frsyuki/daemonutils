#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serve.h"

const char* opt_prog;
const char* opt_serve_dir = SERVE_DIR;

static void usage(const char* msg)
{
	printf(
		"Usage: %s [options] <command> [args ...]\n"
		, opt_prog);

	if(msg) { printf("error: %s\n", msg); }

	exit(1);
}

static int open_ctl(const char* name)
{
	char pathbuf[1024];
	strcpy(pathbuf, opt_serve_dir);
	strcat(pathbuf, "/");
	strcat(pathbuf, name);
	strcat(pathbuf, "/" SERVE_CTL_FILE);

	int ret = open(pathbuf, O_RDONLY | O_NDELAY);
	if(ret < 0) {
		fprintf(stderr, "can't open %s: %s\n", pathbuf, strerror(errno));
	}
	return ret;
}

static int open_stat(const char* name)
{
	char pathbuf[1024];
	strcpy(pathbuf, opt_serve_dir);
	strcat(pathbuf, "/");
	strcat(pathbuf, name);
	strcat(pathbuf, "/" SERVE_STAT_FILE);

	int fd = open(pathbuf, O_RDONLY | O_NDELAY);
	if(fd < 0) {
		fprintf(stderr, "can't open %s: %s\n", pathbuf, strerror(errno));
		exit(1);
	}
	return fd;
}

static int cmd_start(char* name)
{
	int ctl = open_ctl(name);
	write(ctl, "u", 1);
	return 0;
}

static int cmd_stop(char* name)
{
	int ctl = open_ctl(name);
	write(ctl, "d", 1);
	return 0;
}

static int cmd_kill(char* name)
{
	int ctl = open_ctl(name);
	write(ctl, "d", 1);
	return 0;
}

static int cmd_term(char* name)
{
	int ctl = open_ctl(name);
	write(ctl, "d", 1);
	return 0;
}

static int cmd_stat(char* name)
{
	int stat = open_stat(name);
	if(stat < 0) { exit(1); }

	char buf[1024];
	while(1) {
		ssize_t rl = read(stat, buf, sizeof(buf));
		if(rl <= 0) { return 0; }
		char* p = buf;
		do {
			ssize_t wl = write(1, p, rl);
			if(wl <= 0) { return 0; }
			p  += wl;
			rl -= wl;
		} while(rl > 0);
	}
}

int main(int argc, char** argv)
{
	int c;
	char* name;
	char* cmd;

	opt_prog = argv[0];
	while((c = getopt(argc, argv, "s:")) != -1) {
		switch(c) {
		case 's':
			opt_serve_dir = optarg;
			break;

		default:
			usage(NULL);
		}
	}

	if(argc <= 2) { usage(NULL); }

	name = argv[1];
	cmd  = argv[2];
	argv = argv+3;
	argc = argc-3;

#define SUBCMD(CMD) \
		if(strcmp(cmd, #CMD) == 0) { \
			return cmd_##CMD(name); \
		}

	//SUBCMD(scan);
	SUBCMD(start);
	SUBCMD(stop);
	SUBCMD(stat);
	SUBCMD(kill);
	SUBCMD(term);
	//SUBCMD(pause);
	//SUBCMD(cont);

	usage("unknown command");
}
