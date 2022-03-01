
all:
	valgrind ../../lwc lwclib.c+ > lwclib.c
	gcc -c lwclib.c -Wall -Os
	valgrind ../../lwc testlib.c+ > GCC.c
	gcc GCC.c lwclib.o -Wall -Os

clean:
	rm -f lwclib.c GCC.c lwclib.o a.out .preprocfile
