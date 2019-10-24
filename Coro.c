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
	self->stack = calloc(1, CORO_STACK_SIZE + 16);
    printf("Coro_new self=%p, self->stack=%p\n", self, self->stack);
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
    printf("Coro_startCoro_ self=%p,other=%p\n", self, other);
    globalCallbackBlock.context = context;
    globalCallbackBlock.func = callback;
	
	Coro_setup(other);
	Coro_switchTo_(self, other);
}


static void Coro_Start(void)
{
    //printf("In Coro_Start. func=%p, context=%p\n",globalCallbackBlock.func,globalCallbackBlock.context);
	(globalCallbackBlock.func)(globalCallbackBlock.context);
	printf("Scheduler error: returned from coro start function\n");
	exit(-1);
}
 


void Coro_switchTo_(Coro *self, Coro *next)
{
    printf("Coro_switchTo %p\n", next);
    //debug_jmpbuf(next->env);
	if (setjmp(self->env) == 0)
	{
		longjmp(next->env, 1);
	}
}

// ---- setup ------------------------------------------

/* Libc is doing pointer mangling, grep for PTR_MANGLE 
 * in libc */
static uintptr_t ptr_mangle(uintptr_t ptr){
        asm ("xor %%fs:%c2, %0\n"               
             "rol $0x11, %0"           
             : "=r" (ptr)                 
             : "0" (ptr),                 
               "i" (0x30) /* (offsetof (tcbhead_t, pointer_guard)) */);
        return ptr;
}

static void Coro_setup(Coro *self)
{
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
    
    // Let sp point to the top of the stack (self->stack points the the bottom). 
    uintptr_t sp_mangled = ptr_mangle((uintptr_t)self->stack + CORO_STACK_SIZE);
    uintptr_t ip_mangled = ptr_mangle((uintptr_t)Coro_Start);
    printf("Coro_setup  ip=%p, ip_mangled=0x%"PRIx64"\n",
            Coro_Start, ip_mangled);
    printf("            sp=%p, sp_mangled=0x%"PRIx64"\n",
            self->stack, sp_mangled);
    
	setjmp(self->env);
	self->env[0].__jmpbuf[6] = sp_mangled;
	self->env[0].__jmpbuf[7] = ip_mangled;
}
