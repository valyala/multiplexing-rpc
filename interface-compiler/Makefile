CFLAGS=-Wall -fpic -combine -fwhole-program -g -I./include -U_FORTIFY_SOURCE
LDFLAGS=
CC=gcc

SRC_DIR=src

INTERFACE_COMPILER_SRCS= \
	$(SRC_DIR)/c_client_generator.c \
	$(SRC_DIR)/common.c \
	$(SRC_DIR)/c_server_generator.c \
	$(SRC_DIR)/interface.c \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/parser.c

default: all

all: mprc-interface-compiler

mrpc-interface-compiler: $(INTERFACE_COMPILER_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o mrpc-interface-compiler $(INTERFACE_COMPILER_SRCS)

clean:
	rm -f mrpc-interface-compiler

