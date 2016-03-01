CFLAGS=-Wall

tcwebngin:main.o
main.o:main.c client.o server.o config.o
clean:
	@rm -rf tcwebngin *.o