/*
 * daemonutils supervise
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "sig.h"

#define DEV_NULL "/dev/null"

const char* opt_ctl_path = NULL;
const char* opt_stat_path = NULL;

int opt_argc = 0;
char** opt_argv = NULL;

void (*opt_func)(void*) = NULL;
void* opt_func_data = NULL;

int opt_daemonize = 0;

int opt_auto_start = 1;
int opt_auto_restart = 1;

static int g_ctl_rfd = -1;
static int g_ctl_wfd = -1;

static pid_t g_pid;
static int g_flagexit;
static int g_flagwant;
static int g_flagwantup;
static int g_flagpaused;

static void sigchild_handler(int signum)
{
	write(g_ctl_wfd, "s", 1);
}

static void stat_pidchange(void)
{
	// FIXME
}

static void stat_update(void)
{
	// FIXME
	if(!opt_stat_path) { return; }
	int wfd = open(opt_stat_path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if(wfd < 0) {
		// FIXME
		return;
	}

	FILE* f = fdopen(wfd, "w");
	if(!f) {
		// FIXME
		return;
	}

	fprintf(f, "pid=%d\n", g_pid);
	fclose(f);
}

static void trystart(void)
{
	pid_t pid;
fprintf(stderr, "start\n"); // FIXME path

	pid = fork();
	if(pid < 0) {
		fprintf(stderr, "can't fork child process, sleeping 60 seconds: %s\n", strerror(errno)); // FIXME path
		sleep(60);
		return;
	} else if(pid == 0) {
		sig_uncatch(SIGCHLD);
		sig_unblock(SIGCHLD);

		if(opt_func) {
			(*opt_func)(opt_func_data);
			exit(0);
		} else {
			execvp(opt_argv[0], opt_argv);
			fprintf(stderr, "can't execute %s: %s\n", opt_argv[0], strerror(errno)); // FIXME path
			exit(127);
		}
	}

	sleep(1);

	g_pid = pid;
	g_flagpaused = 0;

	stat_pidchange();
	stat_update();
}

int supervise_init(void)
{
	int ret;

	if(opt_ctl_path == NULL) {
		int pair[2];
		ret = pipe(pair);
		if(ret < 0) { return -1; }

		g_ctl_rfd = pair[0];
		g_ctl_wfd = pair[1];

		return 0;
	}

	ret = mkfifo(opt_ctl_path, 0600);
	if(ret < 0) {
		fprintf(stderr, "can't create fifo %s: %s\n", opt_ctl_path, strerror(errno));
		goto err_mkfifo;
	}

	g_ctl_rfd = open(opt_ctl_path, O_RDONLY | O_NDELAY);
	if(g_ctl_rfd < 0) {
		fprintf(stderr, "can't open %s fifo for read: %s\n", opt_ctl_path, strerror(errno));
		goto err_open_rfd;
	}
	ret = fcntl(g_ctl_rfd, F_SETFL, 0);  // unset O_NONBLOCK flag

	g_ctl_wfd = open(opt_ctl_path, O_WRONLY);
	if(g_ctl_wfd < 0) {
		fprintf(stderr, "can't open %s fifo for write: %s\n", opt_ctl_path, strerror(errno));
		goto err_open_wfd;
	}

	return 0;

err_open_wfd:
	close(g_ctl_rfd);

err_open_rfd:
	unlink(opt_ctl_path);

err_mkfifo:
	return -1;
}

void supervise_exit(void)
{
	close(g_ctl_rfd);
	close(g_ctl_wfd);
	if(opt_ctl_path != NULL) {
		unlink(opt_ctl_path);
	}
}

int supervise_run(void)
{
	g_pid = 0;
	g_flagexit = 0;
	g_flagwant = 1;
	g_flagwantup = opt_auto_start;
	g_flagpaused = 0;

	sig_block(SIGCHLD);
	sig_catch(SIGCHLD, sigchild_handler);

	stat_pidchange();
	stat_update();

	if(g_flagwant && g_flagwantup) {
		trystart();
	}

	while(1) {
		char c;
		ssize_t rl;

		if(g_flagexit && !g_pid) { return 0; }

printf("waiting pid pid=%d\n",g_pid);
		while(1) {
			int stat;
			int r = waitpid(-1, &stat, WNOHANG);
			if(r == 0) { break; }
			if(r < 0 && errno != EAGAIN && errno != EINTR) { break; }
			if(r == g_pid) {
				g_pid = 0;
				stat_pidchange();
				stat_update();
				if(g_flagexit) { return 0; }
				if(g_flagwant && g_flagwantup) {
					trystart();
					break;
				}
			}
		}

printf("reading... pid=%d\n",g_pid);
		sig_unblock(SIGCHLD);
		rl = read(g_ctl_rfd, &c, 1);
		if(rl <= 0) {
			if(errno == EAGAIN || errno == EINTR) {
				continue;
			}
			return -1;
		}
		sig_block(SIGCHLD);

		switch(c) {
		case 'd':  /* down */
printf("down %d\n",g_pid);
			g_flagwant = 1;
			g_flagwantup = 0;
			if(g_pid) {
				kill(g_pid, SIGTERM);
				kill(g_pid, SIGCONT);
				g_flagpaused = 0;
			}
			stat_update();
			break;

		case 'u':  /* up */
printf("up %d\n",g_pid);
			g_flagwant = 1;
			g_flagwantup = 1;
			if(!g_pid) { trystart(); }
			stat_update();
			break;

		case 'o':  /* once */
printf("once %d\n",g_pid);
			g_flagwant = 0;
			if(!g_pid) { trystart(); }
			stat_update();
			break;

		case 'x':  /* exit */
printf("exit %d\n",g_pid);
			g_flagexit = 1;
			stat_update();
			break;

		case 'p':  /* pause */
printf("pause %d\n",g_pid);
			g_flagpaused = 1;
			if(g_pid) { kill(g_pid, SIGSTOP); }
			stat_update();
			break;

		case 'c':  /* continue */
printf("continue %d\n",g_pid);
			g_flagpaused = 0;
			if(g_pid) { kill(g_pid, SIGCONT); }
			stat_update();
			break;

		case 'h':  /* hup */
printf("hup %d\n",g_pid);
			if(g_pid) { kill(g_pid, SIGHUP); }
			break;

		case 'a':  /* alarm */
printf("alarm %d\n",g_pid);
			if(g_pid) { kill(g_pid, SIGALRM); }
			break;

		case 'i':  /* interrupt */
printf("interrupt %d\n",g_pid);
			if(g_pid) { kill(g_pid, SIGINT); }
			break;

		case 't':  /* term */
printf("term %d\n",g_pid);
			if(g_pid) { kill(g_pid, SIGTERM); }
			break;

		case 'k':  /* kill */
printf("kill %d\n",g_pid);
			if(g_pid) { kill(g_pid, SIGKILL); }
			break;

		case '1':  /* usr1 */
printf("usr1 %d\n",g_pid);
			if(g_pid) { kill(g_pid, SIGUSR1); }
			break;

		case '2':  /* usr2 */
printf("usr2 %d\n",g_pid);
			if(g_pid) { kill(g_pid, SIGUSR2); }
			break;

		case 's':  /* sigchld */
printf("sigchld %d\n",g_pid);
			if(!opt_auto_restart) {
				g_flagwant = 0;
				stat_update();
			}
			break;

		case ' ': /* ping */
			// FIXME touch status file
			break;
		}
	}
}


static int daemonize(void)
{
	pid_t pid;
	int rnull, wnull;

	rnull = open(DEV_NULL, O_RDONLY);
	if(rnull < 0) {
		fprintf(stderr, "can't open " DEV_NULL " for read: %s\n", strerror(errno));
		return -1;
	}

	wnull = open(DEV_NULL, O_APPEND);
	if(wnull < 0) {
		fprintf(stderr, "can't open " DEV_NULL " for append: %s\n", strerror(errno));
		close(rnull);
		return -1;
	}
	
	pid = fork();
	if(pid < 0) {
		fprintf(stderr, "can't fork child process for daemonize: %s\n", strerror(errno));
		close(wnull);
		close(rnull);
		return -1;
	}
	if(pid > 0) { exit(0); }

	close(0);
	close(1);
	close(2);
	dup2(rnull, 0);
	dup2(wnull, 1);
	dup2(wnull, 2);
	close(rnull);
	close(wnull);

	setsid();
	pid = fork();
	if(pid <  0) { exit(1); }
	if(pid != 0) { exit(0); }

	return 0;
}

static void run()
{
	if(supervise_init() < 0) { return; }

	if(opt_daemonize && daemonize() < 0) {
		supervise_exit();
		exit(1);
	}

	fcntl(g_ctl_rfd, F_SETFD, FD_CLOEXEC);
	fcntl(g_ctl_wfd, F_SETFD, FD_CLOEXEC);

	supervise_run();
	supervise_exit();
}

static void usage(const char* prog, const char* msg)
{
	printf(
		"Usage: %s [options] -- <command> [args ...]\n"
		" -c PATH           : use control fifo\n"
		" -s PATH           : write status to to the file\n"
		" -x                : don't start automatically\n"
		" -d                : daemonize\n"
		, prog);

	if(msg) { printf("error: %s\n", msg); }

	exit(1);
}

int main(int argc, char** argv)
{
	int c;
	while((c = getopt(argc, argv, "c:s:xd")) != -1) {
		switch(c) {
		case 'c':
			opt_ctl_path = optarg;
			break;

		case 's':
			opt_stat_path = optarg;
			break;

		case 'x':
			opt_auto_start = 0;
			break;

		case 'd':
			opt_daemonize = 1;
			break;

		default:
			usage(argv[0], NULL);
		}
	}

	if(argc - optind == 0) {
		usage(argv[0], NULL);
	}

	opt_argc = argc - optind;
	opt_argv = argv + optind;

	run();

	return 0;
}

