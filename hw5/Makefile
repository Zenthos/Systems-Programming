CFLAGS = -g -Wall -Wvla -I inc -D_REENTRANT
LFLAGS = -L lib -lSDL2 -lSDL2_image -lSDL2_ttf -pthread
OUTPUT = client server

%.o: %.c %.h
	gcc $(CFLAGS) -c -o $@ $<

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

all: $(OUTPUT)

runclient:
	LD_LIBRARY_PATH=lib ./client 127.0.0.1 8180

runserver:
	./server.o 8180

client: client.o
	gcc $(CFLAGS) -o $@ $^ $(LFLAGS)

server: server.o
	gcc $(CFLAGS) -o server.o -pthread server.c

clean:
	rm -f $(OUTPUT) *.o
