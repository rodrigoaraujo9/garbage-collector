MAKEFILE = Makefile

GC ?= sweep
EXEC ?= mutator

ifeq ($(GC),sweep)
GCFLAG = -DMARK_SWEEP
else ifeq ($(GC),compact)
GCFLAG = -DMARK_COMPACT
else ifeq ($(GC),copy)
GCFLAG = -DCOPY_COLLECT
else
$(error Invalid GC strategy. Use GC=sweep, GC=compact, or GC=copy)
endif

ifeq ($(EXEC),mutator)
MAIN_SRC = mutator.c
MAIN_OBJ = $(BINDIR)/mutator.o
else ifeq ($(EXEC),toyvm)
MAIN_SRC = toyvm.c
MAIN_OBJ = $(BINDIR)/toyvm.o
else
$(error Invalid EXEC. Use EXEC=mutator or EXEC=toyvm)
endif

BINDIR = bin

COMMON_SRC = collector.c heap.c bistree.c list.c

OBJFLS = $(MAIN_OBJ) \
         $(BINDIR)/collector.o \
         $(BINDIR)/heap.o \
         $(BINDIR)/bistree.o \
         $(BINDIR)/list.o

CC = gcc
LL = gcc

CFLAGS = -Wall $(GCFLAG)
LFLAGS =

$(EXEC): $(OBJFLS)
	$(LL) $(LFLAGS) $(OBJFLS) -o $@

$(BINDIR)/%.o: %.c | $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR):
	mkdir -p $(BINDIR)

all:
	$(MAKE) clean
	$(MAKE) $(EXEC) GC=$(GC) EXEC=$(EXEC)

test: $(EXEC)
	./$(EXEC) 80 10 100000000 100

clean:
	-rm -f mutator toyvm
	-rm -f $(BINDIR)/*.o

$(BINDIR)/mutator.o: bool.h globals.h list.h bistree.h collector.h heap.h mutator.c
$(BINDIR)/toyvm.o: bool.h globals.h list.h bistree.h collector.h heap.h toyvm.c
$(BINDIR)/collector.o: bool.h globals.h list.h bistree.h heap.h collector.h collector.c
$(BINDIR)/heap.o: heap.h globals.h heap.c
$(BINDIR)/bistree.o: bool.h bistree.h bistree.c
$(BINDIR)/list.o: bool.h list.h list.c
