CC ?=($CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt
ifeq ($(USE_AESD_CHAR_DEVICE),0)
  DEVFLAGS = 
else
  DEVFLAGS = -DUSE_AESD_CHAR_DEVICE
endif
#$@ The file name of the target of the rule
#$^ The names of all the prerequisites, with spaces between them
aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES) $(LDFLAGS) $(DEVFLAGS)

clean:
	-rm $(DEST_DIR)aesdsocket 

install:
	cp ./aesdsocket $(DEST_DIR) 
	chmod +x $(DEST_DIR)aesdsocket

all: 
	#$(CC) aesdsocket.c -o ./aesdsocket
	$(CC) $(CFLAGS) $^ aesdsocket.c -o aesdsocket $(INCLUDES) $(LDFLAGS) $(DEVFLAGS)
	chmod +x ./aesdsocket

default:
	#$(CC) aesdsocket.c -o ./aesdsocket
	$(CC) $(CFLAGS) $^ aesdsocket.c -o aesdsocket $(INCLUDES) $(LDFLAGS) $(DEVFLAGS)
