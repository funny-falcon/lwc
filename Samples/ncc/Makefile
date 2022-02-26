
LWC = valgrind ../../lwc
LWC = ../../lwc


CC =gcc -Wall 

all: dbstree.o inttree.o lex.o mem_pool.o space.o cexpand.o preproc.o cdb.o parser.o usage.o ccexpr.o main.c portabill.o
	$(CC) main.c *.o -o ncc  
	size ncc
	@echo Salut
	@echo now ncc will analyse it self. please wait.
	sleep 4
	./ncc *.c > code.map
	@echo Ok. See code.map
	@wc code.map

a: dbstree.o inttree.o lex.o mem_pool.o space.o cexpand.o preproc.o cdb.o parser.o usage.o ccexpr.o main.c
	@echo done

b: dbstree.c inttree.c lex.c mem_pool.c space.c cexpand.c preproc.c cdb.c parser.c usage.c ccexpr.c main.c

main.c: main.c+
	$(LWC) main.c+ > main.c


portabill.o: portabill.cw
	cp portabill.cw portabill.c
	$(CC) portabill.c -c

dbstree.o: dbstree.c
	$(CC) dbstree.c  -c 

dbstree.c: dbstree.c+ dbstree.h
	$(LWC) dbstree.c+ > dbstree.c



inttree.o: inttree.c
	$(CC) inttree.c  -c 

inttree.c: inttree.c+ inttree.h
	$(LWC) inttree.c+ > inttree.c



mem_pool.o: mem_pool.c
	$(CC) mem_pool.c  -c 

mem_pool.c: mem_pool.c+ mem_pool.h
	$(LWC) mem_pool.c+ > mem_pool.c



lex.o: lex.c
	$(CC) lex.c  -c 

lex.c: lex.c+
	$(LWC) lex.c+ > lex.c



space.o: space.c
	$(CC) space.c  -c 

space.c: space.c+
	$(LWC) space.c+ > space.c



cexpand.o: cexpand.c
	$(CC) cexpand.c  -c 

cexpand.c: cexpand.c+
	$(LWC) cexpand.c+ > cexpand.c



preproc.o: preproc.c
	$(CC) preproc.c  -c 

preproc.c: preproc.c+
	$(LWC) preproc.c+ > preproc.c



cdb.o: cdb.c
	$(CC) cdb.c  -c 

cdb.c: cdb.c+
	$(LWC) cdb.c+ > cdb.c



parser.o: parser.c
	$(CC) parser.c  -c 

parser.c: parser.c+
	$(LWC) parser.c+ > parser.c



usage.o: usage.c
	$(CC) usage.c  -c 

usage.c: usage.c+
	$(LWC) usage.c+ > usage.c



ccexpr.o: ccexpr.c
	$(CC) ccexpr.c  -c 

ccexpr.c: ccexpr.c+
	$(LWC) ccexpr.c+ > ccexpr.c



clean:
	rm -f *.c *.o ncc code.map .preprocfile
