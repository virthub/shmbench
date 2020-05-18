#ifndef _VLOCK_H
#define _VLOCK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "dipc.h"

typedef struct shmlock {
    int X;
    int Y;
    int count;
    int total;
    int B[0];
} vlock_t;

#define vlock_vec_size(locks) (locks * PAGE_SIZE)
#define vlock_get(vec, ent) ((vlock_t *)((vec) + (ent) * PAGE_SIZE))
#define vlock_size(nodes) (nodes * sizeof(int) * 2 + sizeof(vlock_t))

void *vlock_check(int desc);
int vlock_find(int key, int locks);
void vlock_release(int desc, void *vec);
void vlock_join(int id, void *vec, int ent);
void vlock_unlock(int id, void *vec, int ent);
int vlock_create(int key, int nodes, int locks);
void *vlock_init(int desc, int nodes, int locks);
void vlock_wait(int id, void *vec, int ent, int total);
void vlock_lock(int id, void *vec, int ent, bool *first, bool *fresh);

#endif
