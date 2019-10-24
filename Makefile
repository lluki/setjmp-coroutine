main: main.c Coro.c
	gcc -g -Wall main.c Coro.c -o main

clean:
	rm main
