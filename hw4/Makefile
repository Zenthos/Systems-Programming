CC       = gcc
CFLAGS   = -g -Wall -Wvla -fsanitize=address
LDFLAGS  =
OBJFILES = memperf.c mymalloc.c
TARGET   = memperf

make:
	${CC} ${CFLAGS} -o ${TARGET} ${OBJFILES} ${LDFLAGS}
