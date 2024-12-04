writer: writer.c
	$(CROSS_COMPILE)gcc writer.c -o writer

clean:
	rm -rf $(DEST_DIR)*.o
	rm $(DEST_DIR)writer

install:
	cp ./writer $(DEST_DIR) 
	chmod +x $(DEST_DIR)writer

all:
	$(CC) writer.c -o writer
