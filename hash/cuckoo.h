// Cuckoo Hash
// Author: Nicholas Sullivan

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hash.h"

#ifndef CUCKOO_H
#define CUCKOO_H

#define CUCKOO_RESIZE_LIM 50
#define CUCKOO_RESIZE_MULT 2
#define CUCKOO_MAX_DEP 100

typedef struct cuckoo_node
{
    uint8_t taken;
    char *key;
    void *val;
} cuckoo_node;

typedef struct Cuckoo
{
    uint32_t cap;
    uint32_t size;
    uint32_t size_lim;
    uint64_t f_seed;
    uint64_t s_seed;
    cuckoo_node *nodes;
}cuckoo;

// init new cuckoo hash table
cuckoo *
cuckoo_init(uint32_t init_cap);

// insert value into cuckoo table
// return 1 if key already exists
uint8_t
cuckoo_insert(cuckoo *cuck, char *key, void *val);

// return true if key exists
uint8_t
cuckoo_exists(cuckoo *cuck, char *key);

// get value from cuckoo table
void *
cuckoo_get(cuckoo *cuck, char *key);

// remove value from cuckoo table
// return void * to data
// caller responsible for freeing data
void *
cuckoo_remove(cuckoo *cuck, char *key);

// destroy cuckoo table
// deep == 1 will free values
void
cuckoo_destroy(cuckoo *cuck, uint8_t deep);

#endif
