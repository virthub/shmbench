#ifndef _BL_H
#define _BL_H

#include <sys/time.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "dipc.h"

#define KEY_MAIN        7100
#define KEY_LOCK        7200
#define PAGE_SIZE       4096
#define BLOCK_MAX       1024
#define BLOCK_SIZE      (1 << 20)
#define BARRIER_START   9100
#define BARRIER_TEST    9200
#define BARRIER_END     9300
#define GUARD_TIME      10   // msec

#define BL_BLOCKS       256
#define BL_BLOCK_SIZE   1024 
#define BL_EP           0.5 // Extending possibility for accessing the contineous blocks
#define BL_ROUNDS       10000
#define BL_RESULT       "/tmp/.bl_result"
#define BL_RELEASE      "/tmp/.bl_release"

#define SHOW_RESULT
// #define SHOW_LOG
// #define SHOW_LOCK
// #define SHOW_BINARY

#ifdef SHOW_LOG
#define bl_log printf
#else
#define bl_log(...) do {} while (0)
#endif

#ifdef SHOW_BINARY
static inline void show_banary(const char *name, void *ptr, size_t size)
{
    int i;
    int j;
    int cnt = 0;
    char *p = ptr;
    const int width = 32;
    char str[width + 1];

    printf("%s:\n", name);
    for (i = 0; i < size; i++) {
        char ch = p[i];

        for (j = 0; j < 8; j++) {
            if (ch & 1)
                str[cnt++] = '1';
            else
                str[cnt++] = '0';
            ch >>= 1;
        }
        if (cnt == width) {
            str[cnt] = '\0';
            printf("%s\n", str);
            cnt = 0;
        }
    }
    if (cnt) {
        str[cnt] = '\0';
        printf("%s\n", str);
    }
}
#else
#define show_binary(...) do {} while (0)
#endif

#endif
