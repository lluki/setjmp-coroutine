/*
 Credits

	Originally based on Edgar Toernig's Minimalistic cooperative multitasking
	http://www.goron.de/~froese/
	reorg by Steve Dekorte and Chis Double
	Symbian and Cygwin support by Chis Double
	Linux/PCC, Linux/Opteron, Irix and FreeBSD/Alpha, ucontext support by Austin Kurahone
	FreeBSD/Intel support by Faried Nawaz
	Mingw support by Pit Capitain
	Visual C support by Daniel Vollmer
	Solaris support by Manpreet Singh
	Fibers support by Jonas Eschenburg
	Ucontext arg support by Olivier Ansaldi
	Ucontext x86-64 support by James Burgess and Jonathan Wright
	Russ Cox for the newer portable ucontext implementions.

 Notes

	This is the system dependent coro code.
	Setup a jmp_buf so when we longjmp, it will invoke 'func' using 'stack'.
	Important: 'func' must not return!

	Usually done by setting the program counter and stack pointer of a new, empty stack.
	If you're adding a new platform, look in the setjmp.h for PC and SP members
	of the stack structure

	If you don't see those members, Kentaro suggests writting a simple
	test app that calls setjmp and dumps out the contents of the jmp_buf.
	(The PC and SP should be in jmp_buf->__jmpbuf).

	Using something like GDB to be able to peek into register contents right
	before the setjmp occurs would be helpful also.
 */

#include "Coro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef struct CallbackBlock
{
	void *context;
	CoroStartCallback *func;
} CallbackBlock;

static CallbackBlock globalCallbackBlock;

Coro *Coro_new(void)
{
	Coro *self = (Coro *)calloc(1, sizeof(Coro));
	self->stack = calloc(1, CORO_STACK_SIZE);
    printf("allocated stack at %p\n", self->stack);
	return self;
}

Coro *Coro_new_main(void)
{
	Coro *self = (Coro *)calloc(1, sizeof(Coro));
	return self;
}

void Coro_free(Coro *self)
{
	if (self->stack)
	{
		free(self->stack);
	}
	free(self);
}

void *Coro_stack(Coro *self)
{
	return self->stack;
}

size_t Coro_bytesLeftOnStack(Coro *self)
{
	unsigned char dummy;
	ptrdiff_t p1 = (ptrdiff_t)(&dummy);
    ptrdiff_t start = ((ptrdiff_t)self->stack);
	ptrdiff_t end   = start + CORO_STACK_SIZE;

    return end - p1;
}


static void Coro_setup(Coro *self);
static void Coro_Start(void);

void Coro_startCoro_(Coro *self, Coro *other, void *context, CoroStartCallback *callback)
{
	//CallbackBlock sblock;
	//CallbackBlock *block = &sblock;
	////CallbackBlock *block = malloc(sizeof(CallbackBlock)); // memory leak
    globalCallbackBlock.context = context;
    globalCallbackBlock.func = callback;
	//block->context = context;
	//block->func    = callback;
	
	Coro_setup(other);
    printf("Coro_Start=%p\n", Coro_Start);
    printf("other->env after Coro_setup\n");
    debug_jmpbuf(other->env);
	Coro_switchTo_(self, other);
}

void Coro_StartWithArg(CallbackBlock *block)
{
	(block->func)(block->context);
	printf("Scheduler error: returned from coro start function\n");
	exit(-1);
}


static void Coro_Start(void)
{
	CallbackBlock block = globalCallbackBlock;
	Coro_StartWithArg(&block);
}
 


void Coro_switchTo_(Coro *self, Coro *next)
{
    printf("switchTo %p\n", next);
    debug_jmpbuf(next->env);
	if (setjmp(self->env) == 0)
	{
		longjmp(next->env, 1);
	}
}

// ---- setup ------------------------------------------

static void Coro_setup(Coro *self)
{
	setjmp(self->env);
	/* This is probably not nice in that it deals directly with
	* something with __ in front of it.
	*
	* Anyhow, Coro.h makes the member env of a struct Coro a
	* jmp_buf. A jmp_buf, as defined in the amd64 setjmp.h
	* is an array of one struct that wraps the actual __jmp_buf type
	* which is the array of longs (on a 64 bit machine) that
	* the programmer below expected. This struct begins with
	* the __jmp_buf array of longs, so I think it was supposed
	* to work like he originally had it, but for some reason
	* it didn't. I don't know why.
	* - Bryce Schroeder, 16 December 2006
	*
	*   Explaination of `magic' numbers: 6 is the stack pointer
	*   (RSP, the 64 bit equivalent of ESP), 7 is the program counter.
	*   This information came from this file on my Gentoo linux
	*   amd64 computer:
	*   /usr/include/gento-multilib/amd64/bits/setjmp.h
	*   Which was ultimatly included from setjmp.h in /usr/include. */
    
    //XX we need to to ROL 11 of this address
	self->env[0].__jmpbuf[6] = ((unsigned long)(Coro_stack(self))) << 11;
	self->env[0].__jmpbuf[7] = (((unsigned long)Coro_Start)) << 11;
}
