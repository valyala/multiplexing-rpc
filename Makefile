CFLAGS=-Wall -fpic -combine -fwhole-program -g -I./include -I./external/fiber-framework/include -DHAS_STDINT_H -D_GNU_SOURCE -U_FORTIFY_SOURCE
LDFLAGS=-lfiber-framework -shared
CC=gcc

SRC_DIR=src

MRPC_LIB_SRCS= \
	$(SRC_DIR)/mrpc_bitmap.c \
	$(SRC_DIR)/mrpc_blob.c \
	$(SRC_DIR)/mrpc_char_array.c \
	$(SRC_DIR)/mrpc_client.c \
	$(SRC_DIR)/mrpc_client_stream_processor.c \
	$(SRC_DIR)/mrpc_int.c \
	$(SRC_DIR)/mrpc_packet.c \
	$(SRC_DIR)/mrpc_packet_stream.c \
	$(SRC_DIR)/mrpc_server.c \
	$(SRC_DIR)/mrpc_server_stream_processor.c \
	$(SRC_DIR)/mrpc_wchar_array.c

default: all

all: libmultiplexing-rpc.so mrpc-interface-compiler mrpc-tests

libfiber-framework.so:
	cd ./external/fiber-framework && make libfiber-framework.so && cp libfiber-framework.so ../../

libmultiplexing-rpc.so: libfiber-framework.so $(MRPC_LIB_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -L. -Wl,--rpath -Wl,. -o libmultiplexing-rpc.so $(MRPC_LIB_SRCS)

mrpc-interface-compiler:
	cd ./interface-compiler && make mrpc-interface-compiler && cp mrpc-interface-compiler ../

mrpc-tests:
	cd ./tests && make mrpc-tests && cp mrpc-tests ../

clean:
	cd ./external/fiber-framework && make clean
	cd ./interface-compiler && make clean
	cd ./tests && make clean
	rm -f libfiber-framework.so libmultiplexing-rpc.so mrpc-interface-compiler mrpc-tests

