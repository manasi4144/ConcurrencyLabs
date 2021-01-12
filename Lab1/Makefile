CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz
 
TARGETS = findpng.c pnginfo.c catpng.c findpng pnginfo catpng is_png.c crc.c crc.h zutil.c zutil.h

all: findpng pnginfo catpng

findpng: findpng
	$(CC) -o findpng findpng.c 

pnginfo: pnginfo
	$(CC) -o pnginfo pnginfo.c

catpng: catpng
	$(CC) -o catpng catpng.c zutil.c $(LDLIBS)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 
