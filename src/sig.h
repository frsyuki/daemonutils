#ifndef SERVE_SIG_H__
#define SERVE_SIG_H__

#include <signal.h>

void sig_catch(int sig, void (*func)(int));
void sig_uncatch(int sig);
void sig_block(int sig);
void sig_unblock(int sig);

#endif /* sig.h */
