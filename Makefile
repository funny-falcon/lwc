
#
#

# enable ncc

CCC = ncc -ncld -ncfabs -ncgcc -Wall -g -Wno-parentheses
CCC = ncc -ncld -ncfabs -ncgcc -Wall -O2 -Wno-parentheses
CCC = gcc -Wall -g -Wno-parentheses

CC = $(CCC) -c

objdir/lwc: objdir/cpp.o objdir/lex.o objdir/dcl.o objdir/cdb.o objdir/fdb.o objdir/misc.o\
	    objdir/util.o objdir/hier.o objdir/rexpr.o objdir/icode.o objdir/textp.o\
	    objdir/inames.o\
            objdir/fspace.o objdir/debugs.o objdir/output.o objdir/except.o objdir/preproc.o\
            objdir/templates.o objdir/breakexpr.o objdir/statement.o objdir/lwc_config.o\
            objdir/regexp.o objdir/main.o
	$(CCC) objdir/*.o -o objdir/lwc
	@echo Done.

force: objdir/lwc
	touch main.c

u: SYS.h
	gcc -O2 -g -Q -Wall cpp.c dcl.c fdb.c lex.c cdb.c misc.c util.c hier.c rexpr.c icode.c fspace.c inames.c debugs.c output.c except.c preproc.c templates.c breakexpr.c statement.c lwc_config.c regexp.c main.c -o objdir/lwc
	size objdir/lwc

s: SYS.h
	34-gcc -Os -g cpp.c dcl.c fdb.c lex.c cdb.c misc.c util.c hier.c rexpr.c icode.c fspace.c inames.c debugs.c output.c except.c preproc.c templates.c breakexpr.c statement.c lwc_config.c regexp.c texp.c main.c -o objdir/lwc
	strip objdir/lwc
	size objdir/lwc

objdir/main.o: main.c global.h
	$(CC) main.c -o objdir/main.o

objdir/icode.o: icode.c global.h
	$(CC) icode.c -o objdir/icode.o

objdir/dcl.o: dcl.c global.h
	$(CC) dcl.c -o objdir/dcl.o

objdir/cpp.o: cpp.c global.h
	$(CC) cpp.c -o objdir/cpp.o

objdir/fdb.o: fdb.c global.h
	$(CC) fdb.c -o objdir/fdb.o

objdir/debugs.o: debugs.c global.h
	$(CC) debugs.c -o objdir/debugs.o

objdir/fspace.o: fspace.c global.h
	$(CC) fspace.c -o objdir/fspace.o

objdir/hier.o: hier.c global.h vtbl.ch
	$(CC) hier.c -o objdir/hier.o

objdir/util.o: util.c global.h
	$(CC) util.c -o objdir/util.o

objdir/statement.o: statement.c global.h
	$(CC) statement.c -o objdir/statement.o

objdir/regexp.o : regexp.c global.h
	$(CC) regexp.c -o objdir/regexp.o

objdir/lwc_config.o : lwc_config.c global.h
	$(CC) lwc_config.c -o objdir/lwc_config.o

objdir/breakexpr.o: breakexpr.c global.h
	$(CC) breakexpr.c -o objdir/breakexpr.o

objdir/rexpr.o: rexpr.c global.h
	$(CC) rexpr.c -o objdir/rexpr.o

objdir/inames.o: inames.c global.h
	$(CC) inames.c -o objdir/inames.o

objdir/preproc.o: preproc.c global.h
	$(CC) preproc.c -o objdir/preproc.o

objdir/except.o: except.c global.h
	$(CC) except.c -o objdir/except.o

objdir/output.o: output.c global.h
	$(CC) output.c -o objdir/output.o

objdir/cdb.o: cdb.c global.h
	$(CC) cdb.c -o objdir/cdb.o

objdir/misc.o: misc.c global.h
	$(CC) misc.c -o objdir/misc.o

objdir/templates.o: templates.c global.h
	$(CC) templates.c -o objdir/templates.o

objdir/textp.o: textp.c global.h
	$(CC) textp.c -o objdir/textp.o

objdir/lex.o: lex.c global.h
	$(CC) lex.c -o objdir/lex.o

global.h: norm.h SYS.h config.h
	touch global.h

clean:
	rm -f objdir/*.o

distclean:
	rm -f objdir/*.o objdir/lwc 
	find . -name .preprocfile | xargs rm -f
	find . -name a.out | xargs rm -f
	find . -name \*.o | xargs rm -f
	find . -name \*.i | xargs rm -f
	find . -name GCC.c | xargs rm -f

ncc:
	ncc *.c > objdir/code.map
	rm NCC.i

n: ncc
	nccnav objdir/code.map
