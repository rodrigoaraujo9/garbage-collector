/*
 * collector.h
 */

#ifndef COLLECTOR_H
#define COLLECTOR_H

void mark_sweep_gc(BisTree* roots);

void mark_compact_gc(BisTree* roots);

void copy_collection_gc(BisTree* roots);

#endif
