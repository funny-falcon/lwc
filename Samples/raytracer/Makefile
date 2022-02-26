
LWFLAGS=
#LCFLAGS = -Wall -O2 -march=athlon-xp -mfpmath=sse -ffast-math -fsingle-precision-constant
LCFLAGS = -Wall -O2 -march=native -mfpmath=sse -ffast-math -fsingle-precision-constant
CFLAGS = -c $(LCFLAGS)

all: grfix.o gauss.o
	../../lwc $(LWFLAGS) main.c+ > main.c
	gcc main.c *.o -Wall -lX11 -L /usr/X11R6/lib -lXext -lm $(LCFLAGS)
	gcc main.c $(LCFLAGS) -S
	@echo You are worthy Raziel

gauss.o: gauss.c
	gcc $(CFLAGS) gauss.c
	gcc gauss.c $(CFLAGS) -S

grfix.o: grfix.c scr.h
	gcc grfix.c $(CFLAGS)

clean:
	rm -f *.o main.c main.s a.out .preprocfile

