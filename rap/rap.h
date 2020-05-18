#ifndef _RAP_H
#define _RAP_H

#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "dipc.h"

#define KEY_MAIN      3500
#define KEY_LOCK      3600
#define BARRIER_START 4000
#define BARRIER_END   5000

#define RAP_ROUNDS    100000
#define RAP_PAGES     256
#define RAP_PAGE_SIZE 4096
#define RAP_RW_RATIO  1

#define RAP_RESULT    "/tmp/.rap_result"
#define RAP_RELEASE   "/tmp/.rap_release"

// #define SHOW_LOG
#define SHOW_RESULT

#ifdef SHOW_LOG
#define rap_log printf
#else
#define rap_log(...) do {} while (0)
#endif

typedef struct {
    int desc;
    char *buf;
} rap_pages_t;

#endif
