CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket
LDFLAGS ?= lpthread -lrt


aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS)  aesdsocket.c -o ./aesdsocket

clean:
	-rm $(DEST_DIR)aesdsocket 

install:
	cp ./aesdsocket $(DEST_DIR) 
	chmod +x $(DEST_DIR)aesdsocket

all:
	$(CC) aesdsocket.c -o ./aesdsocket
	chmod +x ./aesdsocket

default:
	$(CC) aesdsocket.c -o ./aesdsocket
	
