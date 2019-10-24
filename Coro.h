/*
*/

#ifndef CORO_DEFINED
#define CORO_DEFINED 1

#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#include <sys/utsname.h>
#include <inttypes.h>
#include <setjmp.h>

#define nil ((void*)0)
#define nelem(x) (sizeof(x)/sizeof((x)[0]))

#define CORO_STACK_SIZE (8192*16)

typedef struct Coro Coro;

struct Coro
{
	void *stack;
	jmp_buf env;
	unsigned char isMain;
};

Coro *Coro_new(void);
Coro *Coro_new_main(void);
void Coro_free(Coro *self);

// stack

void *Coro_stack(Coro *self);
size_t Coro_bytesLeftOnStack(Coro *self);

void Coro_initializeMainCoro(Coro *self);

typedef void (CoroStartCallback)(void *);

void Coro_startCoro_(Coro *self, Coro *other, void *context, CoroStartCallback *callback);
void Coro_switchTo_(Coro *self, Coro *next);

void debug_jmpbuf(jmp_buf bb);

#endif
