
CC = gcc
CFLAGS = -Wall -std=c99 -g
INCLUDE = -I/usr/include/libxml2

default: compile

compile:
	$(CC) -c $(CFLAGS) -I/usr/include/libxml2 mxtool.c mxutil.c
	$(CC)  -lxml2 mxutil.o mxtool.o -o mxtool

mxtool:
	$(CC) -c $(CFLAGS) -I/usr/include/libxml2 mxtool.c mxutil.c
	$(CC)  -lxml2 mxutil.o mxtool.o -o mxtool

A1:
	$(CC) -c $(CFLAGS) -I/usr/include/libxml2 testProg.c mxutil.c
	$(CC)  -lxml2 testProg.o mxutil.o -o myProg

mxdiff:
	$(CC) -c $(CFLAGS) -I/usr/include/libxml2 mxdiff.c mxutil.c
	$(CC)  -lxml2 mxdiff.o mxutil.o -o diffy

vgcat:
	#valgrind --leak-check=full --show-reachable=yes ./myProg
	valgrind --dsymutil=yes --leak-check=full --show-reachable=yes --suppressions=./vg-zlib.supp ./mxtool -cat collection.xml < trellis.xml > big.xml
vgreview:
	#valgrind --leak-check=full --show-reachable=yes ./myProg
	valgrind --dsymutil=yes --leak-check=full --show-reachable=yes --suppressions=./vg-zlib.supp ./mxtool -review < trellis.xml > newbib.xml
vgkeep:
	#valgrind --leak-check=full --show-reachable=yes ./myProg
	valgrind --dsymutil=yes --leak-check=full --show-reachable=yes --suppressions=./vg-zlib.supp ./mxtool -keep a=Monk < trellis.xml > short.xml
vgdiscard:
	#valgrind --leak-check=full --show-reachable=yes ./myProg
	valgrind --dsymutil=yes --leak-check=full --show-reachable=yes --suppressions=./vg-zlib.supp ./mxtool -discard a=monk < trellis.xml > short.xml
vglib:
	#valgrind --leak-check=full --show-reachable=yes ./myProg
	valgrind --dsymutil=yes --leak-check=full --show-reachable=yes --suppressions=./vg-zlib.supp ./mxtool -lib < trellis.xml 
vgbib:
	#valgrind --leak-check=full --show-reachable=yes ./myProg
	valgrind --dsymutil=yes --leak-check=full --show-reachable=yes --suppressions=./vg-zlib.supp ./mxtool -bib < trellis.xml 


clean:
	rm *.o