#include "Coro.h"
#include "setjmp.h"
#include <stdio.h>

Coro *firstCoro, *secondCoro;

void secondTask(void *context)
{
	int num = 0;
	
	printf("secondTask created with value %d\n", *(int *)context);
	
	while (1) 
	{
		printf("secondTask: %d %d\n", (int)Coro_bytesLeftOnStack(secondCoro), num++);
		Coro_switchTo_(secondCoro, firstCoro);
	}
}

void firstTask(void *context)
{
	int value = 2;
	int num = 0;
	
	printf("firstTask created with value %d\n", *(int *)context);
	secondCoro = Coro_new();
	Coro_startCoro_(firstCoro, secondCoro, (void *)&value, secondTask);
	
	while (1) 
	{
		printf("firstTask:  %d %d\n", (int)Coro_bytesLeftOnStack(firstCoro), num++);
		Coro_switchTo_(firstCoro, secondCoro);
	}
}


void debug_jmpbuf(jmp_buf bb){
    for(int i=0;i<16;i++){
        char * extra = "\t";
        if(i==7) extra = "(rsp)\t";
        if(i==6) extra = "(ip) \t";
        printf("jmpbuf[%d] %s = %"PRIx64"\n", i, extra, bb[0].__jmpbuf[i]);
    }
}

static jmp_buf test;
int main()
{
    printf("heap (apx) \t= %p\n", malloc(16));
    char x;
    printf("main \t= %p\n", main);
    printf("firstTask \t= %p\n", firstTask);
    printf("sp (approximate) = %p\n", &x);
    setjmp(test);
    debug_jmpbuf(test);

	Coro *mainCoro = Coro_new_main();
	int value = 1;
	
	firstCoro = Coro_new();
	Coro_startCoro_(mainCoro, firstCoro, (void *)&value, firstTask);
}
