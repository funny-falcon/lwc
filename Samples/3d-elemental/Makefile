
a.out: grfix.o circuit.c utily.o
	gcc -g -L/usr/X11R6/lib -lm -lX11 circuit.c -O2 grfix.o -Wall utily.o -lXext -fsingle-precision-constant

circuit.c: circuit.c+ global.h
	valgrind ../../lwc circuit.c+ > circuit.c

grfix.o: grfix.c  global.h
	gcc -g -O2 -c grfix.c -Wall

utily.o: utily.c
	gcc -g -O2 -c utily.c -Wall

clean:
	rm -f *.o a.out circuit.c .preprocfile
