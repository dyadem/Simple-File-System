CC=gcc
CFLAGS=-c -g -Wall -Werror

all : test

test: disk.o test_disk.o file.o
	$(CC) disk.o test_disk.o file.o -o test

disk.o: disk.c
	$(CC) $(CFLAGS) disk.c

test_fs.o: test_disk.c
	$(CC) $(CFLAGS) test_disk.c

file.o: file.c
	$(CC) $(CFLAGS) file.c

clean:
	rm -rf *.o test