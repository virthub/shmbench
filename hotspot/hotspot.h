#ifndef _HOTSPOT_H
#define _HOTSPOT_H

#include <sys/time.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "dipc.h"
#include "barrier.h"

#define KEY_MAIN         3235
#define BARRIER_START    4000
#define BARRIER_TEST     5000
#define BARRIER_END      6000

#define HOTSPOT_RW_RATIO 1
#define HOTSPOT_SIZE     262144
#define HOTSPOT_ROUNDS   10000

#define HOTSPOT_RESULT  "/tmp/.hotspot_result"
#define HOTSPOT_RELEASE "/tmp/.hotspot_release"

#define SHOW_RESULT
// #define SHOW_LOG

#ifdef SHOW_LOG
#define hotspot_log printf
#else
#define hotspot_log(...) do {} while (0)
#endif

#endif
