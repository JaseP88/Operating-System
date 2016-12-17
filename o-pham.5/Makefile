CC = gcc
CFLAGS = -g

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: oss process

oss: oss.o  resource.o
	$(CC) -o $@ $(CFLAGS) oss.o resource.o

process: process.o resource.o
	$(CC) -o $@ $(CFLAGS) process.o resource.o

clean:
	rm -f oss process
	rm -f *.o
