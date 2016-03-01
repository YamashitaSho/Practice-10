CFLAGS=-Wall
LOADLIBES = -lm

CC = gcc
OBJS = main.o client.o server.o config.o

tcwebngin:$(OBJS)
	$(CC) -o $@ $(OBJS)

.c.o:
	$(CC) -c $<

clean:
	@rm -rf tcwebngin *.o
