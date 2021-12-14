CC=gcc
FILE=ls
WARN=NO

ifeq (${WARN},YES)
	CFLAGS=-g -Wall -Wvla -fsanitize=address
else
	CFLAGS=
endif

make:
	${CC} ${CFLAGS} -o ${FILE} ${FILE}.c

all:
	${CC} ${CFLAGS} -o ls ls.c
	${CC} ${CFLAGS} -o find find.c
	${CC} ${CFLAGS} -o tree tree.c
