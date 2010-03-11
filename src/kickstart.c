/*
 * daemonutils serve-kickstart
 *
 * Copyright (c) 2009 FURUHASHI Sadayuki
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include "serve.h"
#include "sig.h"

int opt_scan = 0;
const char* opt_supervise = SERVE_SUPERVISE;
#define DEV_NULL "/dev/null"

static int g_rnull;
static int g_wnull;


static void sigchild_handler(int signum)
{
	int stat;
	while(waitpid(-1, &stat, WNOHANG) > 0)
		;
}

static int check_control(const char* path)
{
	int check = open(path, O_WRONLY | O_NDELAY);
	if(check >= 0) {
		fcntl(check, F_SETFL, O_NONBLOCK);
		if(write(check, " ", 1) > 0) {
			close(check);
			return 1;
		}
		close(check);
	}
	unlink(path);
	return 0;
}

static int is_executable(const char* path)
{
	// check it is file and executable
	struct stat st = {};
	stat(path, &st);
	return (st.st_mode & S_IFREG) && (st.st_mode & S_IXUSR);
}

static pid_t fork_run(const char* name)
{
	pid_t pid;

	if(check_control(SERVE_CTL_FILE)) {
		fprintf(stderr, "%s is already running.\n", name);
		return -1;
	}

	pid = fork();
	if(pid < 0) {
		fprintf(stderr, "can't fork child process for %s"SERVE_LOG_RUN_FILE": %s\n", name, strerror(errno));
		return -1;

	} else if(pid == 0) {
		close(0);
		close(1);
		close(2);
		dup2(g_rnull, 0);
		dup2(g_wnull, 1);
		dup2(g_wnull, 2);
		close(g_rnull);
		close(g_wnull);

		sig_uncatch(SIGCHLD);
		setsid();
		pid = fork();
		if(pid <  0) { exit(127); }
		if(pid != 0) { exit(0); }

		char* argv[9];
		argv[0] = (char*)opt_supervise;  // FIXME dup
		argv[1] = (char*)"-x";
		argv[2] = (char*)"-c";
		argv[3] = (char*)SERVE_CTL_FILE;
		argv[4] = (char*)"-s";
		argv[5] = (char*)SERVE_STAT_FILE;
		argv[6] = (char*)"--";
		argv[7] = (char*)"./"SERVE_RUN_FILE;
		argv[8] = (char*)NULL;
		execv(argv[0], argv);
		perror("execv");
		exit(127);
	}

	return pid;
}

static int run_name(const char* name)
{
	int ret;

	if(is_executable(name)) {
		char* pos = (char*)strrchr(name, '.');
		if(pos == NULL || strcmp(pos, ".run") != 0) {
			return -1;
		}

		*pos = '\0';

		char symfile[1024];
		strcpy(symfile, name);
		strcat(symfile, "/" SERVE_RUN_FILE);

		if(is_executable(symfile)) {
			return -1;
		}

		char linkto[1024];
		strcpy(linkto, "../");
		strcat(linkto, name);
		strcat(linkto, ".run");

		mkdir(name, 0755);
		symlink(linkto, symfile);
	}

	ret = chdir(name);
	if(ret < 0) {
		fprintf(stderr, "cannot chdir to %s: %s\n", name, strerror(errno));
		return -1;
	}

	if(!is_executable(SERVE_RUN_FILE)) {
		fprintf(stderr, "%s/"SERVE_RUN_FILE" is not executable.\n", name);
		chdir("..");
		return -1;
	}

	// FIXME check log/run

	pid_t pid = fork_run(name);
	if(pid < 0) {
		chdir("..");
		return -1;
	}

	chdir("..");
	return 0;
}

int kickstart_run(char* path)
{
	int ret;
	char* pos;
	
	pos = strrchr(path, '/');

	if(pos == NULL) {
		return run_name(path);
	}

	*pos = '\0';

	ret = chdir(path);
	if(ret < 0) {
		fprintf(stderr, "can't chdir to %s: %s\n", path, strerror(errno));
		return -1;
	}

	return run_name(pos+1);
}

int kickstart_scan(char* path)
{
	int ret;
	DIR* dir;
	
	ret = chdir(path);
	if(ret < 0) {
		fprintf(stderr, "can't chdir to %s: %s\n", path, strerror(errno));
		return -1;
	}

	dir = opendir(".");
	if(dir == NULL) {
		fprintf(stderr, "can't open directory %s: %s\n", path, strerror(errno));
		return -1;
	}

	while(1) {
		struct dirent* d;
		char* check_strpos;

		d = readdir(dir);
		if(!d) { break; }

		if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
			continue;
		}

		run_name(d->d_name);
	}

	closedir(dir);

	return 0;
}


static int kickstart_init(void)
{
	g_rnull = open(DEV_NULL, O_RDONLY);

	if(g_rnull < 0) {
		fprintf(stderr, "can't open %s for read: %s\n", DEV_NULL, strerror(errno));
		return -1;
	}

	g_wnull = open(DEV_NULL, O_APPEND);
	if(g_wnull < 0) {
		fprintf(stderr, "can't open %s for append: %s\n", DEV_NULL, strerror(errno));
		close(g_rnull);
		return -1;
	}

	return 0;
}


static void usage(const char* prog, const char* msg)
{
	printf(
		"Usage: %s [options] <path>\n"
		" -s                : scan child directories\n"
		" -c COMMAND        : use COMMAND insteadof serve-suprevise command\n"
		, prog);

	if(msg) { printf("error: %s\n", msg); }

	exit(1);
}


int main(int argc, char** argv)
{
	int c;
	while((c = getopt(argc, argv, "sc:")) != -1) {
		switch(c) {
		case 's':
			opt_scan = 1;
			break;

		case 'c':
			opt_supervise = optarg;
			break;

		default:
			usage(argv[0], NULL);
		}
	}

	argc -= optind;

	if(argc != 1) {
		usage(argv[0], NULL);
	}

	char* path = argv[optind];

	sig_catch(SIGCHLD, sigchild_handler);

	int ret;
	ret = kickstart_init();
	if(ret < 0) { exit(1); }

	if(opt_scan) {
		kickstart_scan(path);
	} else {
		kickstart_run(path);
	}

	return 0;
}

