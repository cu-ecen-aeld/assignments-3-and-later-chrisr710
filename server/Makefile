aesdsocket: aesdsocket.c
	$(CROSS_COMPILE)gcc aesdsocket.c -o ./aesdsocket

clean:
	-rm ./aesdsocket 

install:
	cp ./aesdsocket $(DEST_DIR) 
	chmod +x $(DEST_DIR)aesdsocket

all:
	$(CC) aesdsocket.c -o ./aesdsocket
	chmod +x ./aesdsocket

default:
	$(CC) aesdsocket.c -o /tmp/aesdsocket
	touch /tmp/hello
	
