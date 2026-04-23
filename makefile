# *  Define the name of the makefile.                                        *

MAKNAM = makefile

# *  Define the directories in which to search for library files.            *

LIBDRS =

# *  Define the directories in which to search for include files.            *

INCDRS =

# *  Define the library files.                                               *

LIBFLS =

# *  Define the source files.                                                *

SRCFLS = mutator.c\
         collector.c\
         heap.c\
         bistree.c\
         list.c

# *  Define the object files.                                                *

BINDIR = bin

OBJFLS = $(BINDIR)/mutator.o\
         $(BINDIR)/collector.o\
         $(BINDIR)/heap.o\
         $(BINDIR)/bistree.o\
         $(BINDIR)/list.o

# *  Define the executable.                                                  *

EXE    = mutator

# *  Define the compile and link options.                                    *

CC     = gcc
LL     = gcc
CFLAGS = -Wall
LFLAGS =

# *  Define the rules.                                                       *

$(EXE): $(OBJFLS)
	$(LL) $(LFLAGS) $(OBJFLS) -o $@ $(LIBDRS) $(LIBFLS)

$(BINDIR)/%.o: %.c | $(BINDIR)
	$(CC) $(CFLAGS) $(INCDRS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)

all:
	make -f $(MAKNAM) clean
	make -f $(MAKNAM) $(EXE)

clean:
	-rm -f $(EXE)
	-rm -f $(OBJFLS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

$(BINDIR)/mutator.o: bool.h globals.h list.h bistree.h bistree.c
$(BINDIR)/heap.o: heap.h globals.h heap.c
$(BINDIR)/bistree.o: bool.h bistree.h bistree.c
$(BINDIR)/list.o: bool.h list.h list.c
