
a.out: testgd.c graphdevice.c
	gcc -g graphdevice.c testgd.c -Wall -lX11 -L/usr/X11R6/lib

testgd.c: testgd.c+ graphdevice.h
	../../lwc testgd.c+ > testgd.c

graphdevice.c: graphdevice.c+
	../../lwc graphdevice.c+ > graphdevice.c

clean:
	rm -f *.i a.out *.o *.c .preprocfile
