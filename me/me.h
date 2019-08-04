#ifndef _ME_H
#define _ME_H

#include <sys/time.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "dipc.h"

#define KEY_MAIN	 	3238
#define KEY_LOCK		3239
#define BARRIER_START   4000
#define BARRIER_TEST    5000
#define BARRIER_END     6000

#define ME_ROUNDS		10000
#define ME_RESULT       "/tmp/.me_result"
#define ME_RELEASE      "/tmp/.me_release"

#define SHOW_RESULT
// #define SHOW_LOG
// #define SHOW_LOCK
// #define SHOW_BINARY

#ifdef SHOW_LOG
#define me_log printf
#else
#define me_log(...) do {} while (0)
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
