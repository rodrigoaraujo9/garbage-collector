MAKNAM = makefile

GC ?= sweep

ifeq ($(GC),sweep)
GCFLAG = -DMARK_SWEEP
else ifeq ($(GC),compact)
GCFLAG = -DMARK_COMPACT
else ifeq ($(GC),copy)
GCFLAG = -DCOPY_COLLECT
else
$(error Invalid GC strategy. Use GC=sweep, GC=compact, or GC=copy)
endif

LIBDRS =
INCDRS =
LIBFLS =

SRCFLS = mutator.c\
         collector.c\
         heap.c\
         bistree.c\
         list.c

BINDIR = bin

OBJFLS = $(BINDIR)/mutator.o\
         $(BINDIR)/collector.o\
         $(BINDIR)/heap.o\
         $(BINDIR)/bistree.o\
         $(BINDIR)/list.o

EXE    = mutator

CC     = gcc
LL     = gcc
CFLAGS = -Wall $(GCFLAG)
LFLAGS =

$(EXE): $(OBJFLS)
	$(LL) $(LFLAGS) $(OBJFLS) -o $@ $(LIBDRS) $(LIBFLS)

$(BINDIR)/%.o: %.c | $(BINDIR)
	$(CC) $(CFLAGS) $(INCDRS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)

all:
	make -f $(MAKNAM) clean
	make -f $(MAKNAM) $(EXE) GC=$(GC)

test: $(EXE)
	./$(EXE) 80 10 100000000

clean:
	-rm -f $(EXE)
	-rm -f $(OBJFLS)

$(BINDIR)/mutator.o: bool.h globals.h list.h bistree.h collector.h heap.h mutator.c
$(BINDIR)/collector.o: bool.h globals.h list.h bistree.h heap.h collector.h collector.c
$(BINDIR)/heap.o: heap.h globals.h heap.c
$(BINDIR)/bistree.o: bool.h bistree.h bistree.c
$(BINDIR)/list.o: bool.h list.h list.c
