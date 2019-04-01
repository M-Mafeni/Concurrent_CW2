/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>


// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"


// Include functionality relating to the kernel.

#include "lolevel.h"
#include     "int.h"

//struct for Process Control Block
typedef int pid_t;
typedef int sem_t;
typedef enum {
  STATUS_CREATED,
  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING,
  STATUS_TERMINATED
} status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t ;

typedef struct  pcb_t{
     pid_t    pid;
  status_t status;
     ctx_t    ctx;
     int priority;
     int priority_change;
     int waitingTime;
} pcb_t;

//contains process ID and semaphore pointer
//used to check if value sempahore points to has changed
typedef struct waitingProcess{
    pid_t pid;
    sem_t* semaphore;
} waitingProcess;
#endif
