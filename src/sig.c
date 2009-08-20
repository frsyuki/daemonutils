#include <stddef.h>
#include "sig.h"

void sig_catch(int sig, void (*func)(int))
{
//#ifdef HASSIGACTION
	struct sigaction sa;
	sa.sa_handler = func;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
//#else
//	signal(sig,f); /* won't work under System V, even nowadays---dorks */
//#endif
}

void sig_uncatch(int sig)
{
	sig_catch(SIGCHLD, SIG_DFL);
}

void sig_block(int sig)
{
//#ifdef HASSIGPROCMASK
	sigset_t ss;
	sigemptyset(&ss);
	sigaddset(&ss,sig);
	sigprocmask(SIG_BLOCK, &ss, NULL);
//#else
//	sigblock(1 << (sig - 1));
//#endif
}

void sig_unblock(int sig)
{
//#ifdef HASSIGPROCMASK
	sigset_t ss;
	sigemptyset(&ss);
	sigaddset(&ss, sig);
	sigprocmask(SIG_UNBLOCK, &ss, NULL);
//#else
//	sigsetmask(sigsetmask(~0) & ~(1 << (sig - 1)));
//#endif
}


