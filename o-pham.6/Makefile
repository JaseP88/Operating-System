CC = gcc
CFLAGS = -g

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: oss process

oss: oss.o header.o
	$(CC) -o  $@ $(CFLAGS) oss.o header.o

process: process.o header.o
	$(CC) -o  $@ $(CFLAGS) process.o header.o

clean:
	rm -f oss process disk log
	rm -f *.o
