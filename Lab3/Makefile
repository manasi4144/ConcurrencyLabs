CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz
 
TARGETS = producer.c consumer.c extra.c paster2.h paster2.c paster2 catpng.c is_png.c crc.c crc.h zutil.c zutil.h

all: paster2

paster2: paster2.c
	$(CC) -o paster2 paster2.c producer.c consumer.c extra.c catpng.c zutil.c -pthread -lcurl $(LDLIBS)

.PHONY: clean
clean:
	rm -f *.d *.o o* all.png paster2
