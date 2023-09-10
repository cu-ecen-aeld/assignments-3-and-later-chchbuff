CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror

all: writer

writer: writer.o
	$(CC) $^ -o $@

writer.o: writer.c
	$(CC) -c $^ $(CFLAGS) -o $@ 

clean:
	-rm *.o writer

