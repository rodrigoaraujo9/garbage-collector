/*
 * globals.h
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "bistree.h"
#include "heap.h"
#include "time.h"

BisTree *roots;
Heap *heap;

int max_roots;
clock_t gc_ticks;

#endif
