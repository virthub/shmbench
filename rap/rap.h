#ifndef _RAP_H
#define _RAP_H

#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "dipc.h"

#define KEY_AREA      3500
#define KEY_LOCK      3600
#define BARRIER_START 4000
#define BARRIER_TEST  5000
#define BARRIER_END   6000

#define RAP_CONT      1
#define RAP_ROUNDS    10000
#define RAP_AREAS     15

#define RAP_RESULT    "/tmp/.rap_result"
#define RAP_RELEASE   "/tmp/.rap_release"

#define SHOW_RESULT
// #define SHOW_LOG

#ifdef SHOW_LOG
#define rap_log printf
#else
#define rap_log(...) do {} while (0)
#endif

typedef struct rap_lock {
	shmlock_t *lock;
	int desc;
} rap_lock_t;

typedef struct rap_area {
	rap_lock_t lock;
	char *buf;
	int desc;
	int size;
} rap_area_t;

typedef struct rap_areas {
	int desc;
	int flags;
	int count;
	char *buf;
	rap_area_t areas[RAP_AREAS];
} rap_areas_t;

#endif
