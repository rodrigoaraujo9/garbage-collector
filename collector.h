/*
 * collector.h
 */

#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <stddef.h>

void mark_sweep_gc(void *objects, int n_objects, size_t size_of_object);
void mark_compact_gc(void *objects, int n_objects, size_t size_of_object);
void copy_collection_gc(void *objects, int n_objects, size_t size_of_object);

#endif
