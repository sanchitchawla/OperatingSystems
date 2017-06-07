CC=gcc
AR = ar
ARFLAGS = ru
RANLIB = ranlib
CFLAGS= -g
SRCS= client.c server.c
LIBS = -L./lib/

all:: socketlib client server example_thread threadpool_test
#	# strip client
#	# strip server
#	# strip example_thread
#	# strip threadpool_test

socketlib:
	cd lib && make

example_thread: example_thread.o
	$(CC) $(CFLAGS) -o example_thread example_thread.o -lpthread

client: client.o common.o
	$(CC) $(CFLAGS) -o client client.o common.o $(LIBS) -lsock -lpthread

server: server.o common.o threadpool.o
	$(CC) $(CFLAGS) -o server server.o threadpool.o common.o $(LIBS) -lsock -lpthread

threadpool_test: threadpool_test.o threadpool.o
	$(CC) $(CFLAGS) -o threadpool_test threadpool_test.o threadpool.o -lpthread

client.o: client.c common.h
	$(CC) $(CFLAGS) -o client.o -c client.c

server.o: server.c common.h
	$(CC) $(CFLAGS) -o server.o -c server.c

common.o: common.c
	$(CC) $(CFLAGS) -o common.o -c common.c

example_thread.o: example_thread.c
	$(CC) $(CFLAGS) -o example_thread.o -c example_thread.c

threadpool.o: threadpool.c
	$(CC) $(CFLAGS) -o threadpool.o -c threadpool.c

threadpool_test.o: threadpool_test.c threadpool.h
	$(CC) $(CFLAGS) -o threadpool_test.o -c threadpool_test.c

clean:
	/bin/rm -f mtserver.zip
	/bin/rm -f client server example_thread threadpool_test *.o core *~ #*
	cd lib && make clean

zip: clean
	/usr/bin/zip mtserver.zip ./* ./lib/*
