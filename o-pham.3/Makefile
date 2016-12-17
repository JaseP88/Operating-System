##Jason Pham HW3

CC = cc
OPS = -g

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: master slave

master: master.o monitor.o
	gcc -o master master.o monitor.o

slave: slave.o monitor.o
	gcc -o slave slave.o monitor.o

clean:
	rm -f slave master cstest
	rm -f *.o
