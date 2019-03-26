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
    chopstick left;
    chopstick right;
} philosopher;
#endif
