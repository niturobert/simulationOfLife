CC=clang
CFLAGS=-O2 -lm -lSDL2 -march=native -Wall
compile: src/main.c
	${CC} ${CFLAGS} src/main.c -o main.exe
