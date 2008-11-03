CFLAGS=-Wall -fpic -g -I./include -I./external/fiber-framework/include -DHAS_STDINT_H -DHAS_THREAD_KEYWORD -D_GNU_SOURCE
LDFLAGS=-shared
CC=gcc

SRC_DIR=src
TESTS_DIR=tests

MRPC_LIB_OBJS= \
	$(SRC_DIR)/mrpc_bitmap.o \
	$(SRC_DIR)/mrpc_blob.o \
	$(SRC_DIR)/mrpc_blob_param.o \
	$(SRC_DIR)/mrpc_blob_serialization.o \
	$(SRC_DIR)/mrpc_char_array.o \
	$(SRC_DIR)/mrpc_char_array_param.o \
	$(SRC_DIR)/mrpc_char_array_serialization.o \
	$(SRC_DIR)/mrpc_client.o \
	$(SRC_DIR)/mrpc_client_request_processor.o \
	$(SRC_DIR)/mrpc_client_stream_processor.o \
	$(SRC_DIR)/mrpc_data.o \
	$(SRC_DIR)/mrpc_interface.o \
	$(SRC_DIR)/mrpc_int_param.o \
	$(SRC_DIR)/mrpc_int_serialization.o \
	$(SRC_DIR)/mrpc_method.o \
	$(SRC_DIR)/mrpc_packet.o \
	$(SRC_DIR)/mrpc_packet_stream.o \
	$(SRC_DIR)/mrpc_packet_stream_factory.o \
	$(SRC_DIR)/mrpc_param.o \
	$(SRC_DIR)/mrpc_server.o \
	$(SRC_DIR)/mrpc_server_request_processor.o \
	$(SRC_DIR)/mrpc_server_stream_processor.o \
	$(SRC_DIR)/mrpc_wchar_array.o \
	$(SRC_DIR)/mrpc_wchar_array_param.o \
	$(SRC_DIR)/mrpc_wchar_array_serialization.o

ALL_OBJS= \
	$(MRPC_LIB_OBJS)

default: all

all: libfiber-framework libmultiplexing-rpc.so

libfiber-framework:
	cd ./external/fiber-framework && make libfiber-framework.so

libmultiplexing-rpc.so: $(MRPC_LIB_OBJS)
	$(CC) $(MRPC_LIB_OBJS) $(LDFLAGS) -lfiber-framework -L./external/fiber-framework -Wl,--rpath -Wl,./external/fiber-framework -o $@

clean:
	cd ./external/fiber-framework && make clean
	rm -f $(ALL_OBJS) libmultiplexing-rpc.so

