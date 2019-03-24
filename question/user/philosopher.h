#ifndef __PHILOSOPHER_H
#define __PHILOSOPHER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>
#include "PL011.h"

#include "libc.h"
typedef enum{
    EATING,
    THINKING
} state;
typedef enum{
    DIRTY,
    CLEAN
} chopstick;
typedef struct philosopher{
    state state;
    bool hasLeftFork;
    bool hasRightFork;
} philosopher;
//place fork array in shared memory address
/*
Shared Mem = forks
fork forks[16];
pass forks through pipes
use pipe
2d array of pipes
 */
 /*
 pipe struct
 pcb source;
 pcb dest;
 void *data;
 */

#endif
