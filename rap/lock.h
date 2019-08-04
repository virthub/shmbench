#ifndef _LOCK_H
#define _LOCK_H

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
} shmlock_t;

#define shmlock_size(total) (total * sizeof(int) + sizeof(shmlock_t))

shmlock_t *shmlock_check(int desc);
int shmlock_find(int key, int total);
int shmlock_create(int key, int total);
void shmlock_join(int id, shmlock_t *lock);
void shmlock_lock(int id, shmlock_t *lock);
shmlock_t *shmlock_init(int desc, int total);
void shmlock_unlock(int id, shmlock_t *lock);
void shmlock_release(int desc, shmlock_t *lock);
void shmlock_wait(int id, shmlock_t *lock, int total);

#endif
