##Jason Pham HW2

CC = cc
OPS = -g

all: master slave

master: master.c
	$(CC) $(OPS) -o master master.c

slave: slave.c
	$(CC) $(OPS) -o slave slave.c

clean:
	rm -f slave master
	rm -f *.o
