CFLAGS=-Wall -fpic -combine -fwhole-program -g -I../include -I../external/fiber-framework/include -DHAS_STDINT_H
LDFLAGS=-lfiber-framework -lmultiplexing-rpc
CC=gcc

SRC_DIR=.

TESTS_SRCS= \
	$(SRC_DIR)/tests.c

default: all

all: mrpc-tests

libfiber-framework.so:
	cd .. && make libfiber-framework.so && cp libfiber-framework.so ./tests

libmultiplexing-rpc.so:
	cd .. && make libmultiplexing-rpc.so && cp libmultiplexing-rpc.so ./tests

mrpc-tests: libfiber-framework.so libmultiplexing-rpc.so $(TESTS_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -L. -Wl,--rpath -Wl,. -o mrpc-tests $(TESTS_SRCS)

clean:
	rm -f libfiber-framework.so libmultiplexing-rpc.so mrpc-tests

