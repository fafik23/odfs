#on gcc version 4.8.4 (Ubuntu 4.8.4-2ubuntu1~14.04)
#   gcc -pthread -O2 -g -p -D_FILE_OFFSET_BITS=64 -Wall -pthread 
#   odfs.o fifo.o process_path.o -o odfs -l lmdb -L liblmdb/ -ldl -lfuse
#
#   gcc odfs_name.c -o odfs_name.so -shared -DPIC -fPIC -D_FILE_OFFSET_BITS=64 -Wall

CC	= gcc
W	=
THREADS = -pthread
OPT = -O2 -g3 -p -D_FILE_OFFSET_BITS=64 -Wall
CFLAGS	= $(THREADS) $(OPT) $(W) $(XCFLAGS)
LDLIBS	=
SOLIBS	=
LDFLAGS = -lfuse -l lmdb -pthread -L liblmdb/ -ldl
prefix	= /usr/local

########################################################################

PROGS	= odfs
PLUGIN	= odfs_name.so
all:	$(PROGS) $(PLUGIN)

clean:
	rm -rf $(PROGS) *.[ao] *.so *~ testdb

odfs_name.so:	odfs_name.c
	$(CC)  odfs_name.c -o $@ -shared -DPIC -fPIC -D_FILE_OFFSET_BITS=64 -Wall

odfs:	odfs.o fifo.o
	$(CC) $(CFLAGS) $(LDFLAGS) odfs.o fifo.o $(LDLIBS) -o odfs

odfs.o:	odfs.c 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c odfs.c

fifo.o:	fifo.c 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c fifo.c

