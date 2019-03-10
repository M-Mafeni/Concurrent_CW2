/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"
pcb_t pcb[ 2 ]; pcb_t* current = NULL;

void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

    PL011_putc( UART0, '[',      true );
    PL011_putc( UART0, prev_pid, true );
    PL011_putc( UART0, '-',      true );
    PL011_putc( UART0, '>',      true );
    PL011_putc( UART0, next_pid, true );
    PL011_putc( UART0, ']',      true );

    current = next;                             // update   executing index   to P_{next}

  return;
}
//schedule based on priority
void schedule_priority(ctx_t* ctx){
    int length = sizeof(pcb) / sizeof(pcb[0]);
    for(int i = 0; i < length; i++){
        if(current->pid == pcb[i].pid){
            int next = (i + 1) % length;
            dispatch(ctx, &pcb[i], &pcb[next]);
            for(int j = 0; j < length; j++){
                if(j != next){
                    pcb[next].status = STATUS_READY;
                }
            }
            pcb[next].status = STATUS_EXECUTING;
            break;
        }
    }
    return;
}

void schedule_round_robin( ctx_t* ctx ) {
  if     ( current->pid == pcb[ 0 ].pid ) {
    dispatch( ctx, &pcb[ 0 ], &pcb[ 1 ] );      // context switch P_1 -> P_2

    pcb[ 0 ].status = STATUS_READY;             // update   execution status  of P_1
    pcb[ 1 ].status = STATUS_EXECUTING;         // update   execution status  of P_2
  }
  else if( current->pid == pcb[ 1 ].pid ) {
    dispatch( ctx, &pcb[ 1 ], &pcb[ 0 ] );      // context switch P_2 -> P_1

    pcb[ 1 ].status = STATUS_READY;             // update   execution status  of P_2
    pcb[ 0 ].status = STATUS_EXECUTING;         // update   execution status  of P_1
  }

  return;
}

extern void     main_P3();
extern uint32_t tos_P3;
extern void     main_P4();
extern uint32_t tos_P4;

void hilevel_handler_rst( ctx_t* ctx              ) {
  /* Initialise two PCBs, representing user processes stemming from execution
   * of two user programs.  Note in each case that
   *
   * - the CPSR value of 0x50 means the processor is switched into USR mode,
   *   with IRQ interrupts enabled, and
   * - the PC and SP values match the entry point and top of stack.
   */

   memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_1
   pcb[ 0 ].pid      = 1;
   pcb[ 0 ].status   = STATUS_CREATED;
   pcb[ 0 ].ctx.cpsr = 0x50;
   pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_P3 );
   pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_P3  );

   memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );     // initialise 1-st PCB = P_2
   pcb[ 1 ].pid      = 2;
   pcb[ 1 ].status   = STATUS_CREATED;
   pcb[ 1 ].ctx.cpsr = 0x50;
   pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
   pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P4  );

 TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
 TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
 TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
 TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
 TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

 GICC0->PMR          = 0x000000F0; // unmask all            interrupts
 GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
 GICC0->CTLR         = 0x00000001; // enable GIC interface
 GICD0->CTLR         = 0x00000001; // enable GIC distributor

 /* Once the PCBs are initialised, we arbitrarily select the one in the 0-th
  * PCB to be executed: there is no need to preserve the execution context,
  * since it is is invalid on reset (i.e., no process will previously have
  * been executing).
  */
 dispatch( ctx, NULL, &pcb[ 0 ] );
 int_enable_irq();
  return;
}

void hilevel_handler_irq(ctx_t* ctx) {
    // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_TIMER0 ) {
    schedule_priority(ctx); TIMER0->Timer1IntClr = 0x01;
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;
  return;
}

void hilevel_handler_svc(ctx_t* ctx,uint32_t id) {
    switch(id){
        case 0x01 : {  //write call
            int   fd = ( int   )( ctx->gpr[ 0 ] );
            char*  x = ( char* )( ctx->gpr[ 1 ] );
            int    n = ( int   )( ctx->gpr[ 2 ] );
            for( int i = 0; i < n; i++ ) {
            PL011_putc( UART0, *x++, true );
            }
            ctx->gpr[ 0 ] = n;
            break;
        }

        default : { //case 0x0?
            break;
        }
    }
    return;
}
