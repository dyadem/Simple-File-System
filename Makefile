CC=gcc
CFLAGS=-c -g -Wall -Werror

all : run

run: io/disk.o apps/test_disk.o io/file.o
	$(CC) io/disk.o apps/test_disk.o io/file.o -o run

disk.o: io/disk.c
	$(CC) $(CFLAGS) io/disk.c

test_fs.o: apps/test_disk.c
	$(CC) $(CFLAGS) apps/test_disk.c

file.o: io/file.c
	$(CC) $(CFLAGS) io/file.c

clean:
	rm -rf io/*.o apps/*.o *.o run