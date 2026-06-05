#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <stddef.h>

void mark_sweep_gc(void *objects, int n_objects);
void mark_compact_gc(void *objects, int n_objects);
void copy_collection_gc(void *objects, int n_objects);

#endif
