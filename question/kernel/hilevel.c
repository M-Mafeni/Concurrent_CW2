/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"
pcb_t pcb[ 3 ]; pcb_t* current = NULL;
int length = sizeof(pcb) / sizeof(pcb[0]);

//reset priority, add priorities
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

void terminate_process(){
    int length = sizeof(pcb) / sizeof(pcb[0]);
    for(int i = 0; i < length; i++){
        if(current->pid == pcb[i].pid){
            pcb[i].status = STATUS_TERMINATED;
        }
    }
}
//checks whether a process has been terminated
int is_terminated(pcb_t process){
    return process.status == STATUS_TERMINATED;
}
//schedule based on round-robin method
void schedule_round_robin(ctx_t* ctx){
    int next;
    for(int i = 0; i < length; i++){
        if(current->pid == pcb[i].pid){
            int next = (i + 1) % length;
            while(next != i){
                if(!is_terminated(pcb[next])){
                    dispatch(ctx, &pcb[i], &pcb[next]);
                    break;
                }
                next = (next + 1) % length;
            }
            for(int j = 0; j < length; j++){
                if(j != next && !is_terminated(pcb[j])){
                    pcb[j].status = STATUS_READY;
                }
            }
            pcb[next].status = STATUS_EXECUTING;
            break;
        }
    }
    return;
}
//returns the index of the pcb block with the highest priority
// that's not terminated
int getMax(){
    int max_priority = -1;
    int max_index = -1;
    for( int i = 0; i < length; i++){
        if(pcb[i].priority > max_priority && !is_terminated(pcb[i])){
            max_priority = pcb[i].priority;
            max_index = i;
        }
    }
    return max_index;
}
void schedule_priority(ctx_t* ctx){
    int max = getMax();
    //int current_id = -1;
    //possibly decrease the priority of the current block
    for(int i = 0; i < length; i++){
        if(current->pid == pcb[i].pid){
            dispatch(ctx, &pcb[i], &pcb[max]);
            pcb[max].priority -= pcb[max].priority_change;
            pcb[max].status = STATUS_EXECUTING;
            break;
        }
    }
    //increase the priorities of all the blocks that weren't picked
    for(int j =0; j< length; j++){
        if(j != max && !is_terminated(pcb[j])){
            pcb[j].status = STATUS_READY;
            pcb[j].priority += pcb[j].priority_change;
        }
    }
    return;
}
int partition(pcb_t pcb[length],int low,int high){
    pcb_t pivot = pcb[high];
    int i = low - 1;
    for(int j = low; j <= high; j++){
        if(pcb[j].priority > pivot.priority){
            i++;
            pcb_t temp = pcb[i];
            pcb[i] = pcb[j];
            pcb[j] = temp;
        }
    }
    pcb_t temp = pcb[i + 1];
    pcb[i + 1] = pcb[high];
    pcb[high] = temp;
    return i + 1;
}
void quicksort(pcb_t pcb[length],int low,int high){
    if(low < high){
        int split = partition(pcb,low,high);
        quicksort(pcb,low,split - 1);
        quicksort(pcb,split + 1,high);
    }
}

void sort(pcb_t pcb[length]){
    quicksort(pcb,0,length - 1);
}

extern void     main_P3();
extern uint32_t tos_P3;
extern void     main_P4();
extern uint32_t tos_P4;
extern void     main_P5();
extern uint32_t tos_P5;

void hilevel_handler_rst( ctx_t* ctx              ) {
    /* Initialises PCBs, representing user processes stemming from execution
    * of user programs.  Note in each case that
    *
    * - the CPSR value of 0x50 means the processor is switched into USR mode,
    *   with IRQ interrupts enabled, and
    * - the PC and SP values match the entry point and top of stack.
    */
    PL011_putc(UART0,'R',true);
    memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_3
    pcb[ 0 ].pid      = 1;
    pcb[ 0 ].status   = STATUS_CREATED;
    pcb[ 0 ].ctx.cpsr = 0x50;
    pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_P3 );
    pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_P3  );
    pcb[ 0 ].priority_change = 1;
    pcb[ 0 ].priority = 20;

    memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );     // initialise 1-st PCB = P_4
    pcb[ 1 ].pid      = 2;
    pcb[ 1 ].status   = STATUS_CREATED;
    pcb[ 1 ].ctx.cpsr = 0x50;
    pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P4 );
    pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P4  );
    pcb[ 1 ].priority_change = 2;
    pcb[ 1 ].priority = 20;


    memset( &pcb[ 2 ], 0, sizeof( pcb_t ) );     // initialise 2-nd PCB = P_5
    pcb[ 2 ].pid      = 3;
    pcb[ 2 ].status   = STATUS_CREATED;
    pcb[ 2 ].ctx.cpsr = 0x50;
    pcb[ 2 ].ctx.pc   = ( uint32_t )( &main_P5 );
    pcb[ 2 ].ctx.sp   = ( uint32_t )( &tos_P5  );
    pcb[ 2 ].priority_change = 10;
    pcb[ 2 ].priority = 20;


    TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
    TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
    TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
    TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
    TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

    GICC0->PMR          = 0x000000F0; // unmask all            interrupts
    GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
    GICC0->CTLR         = 0x00000001; // enable GIC interface
    GICD0->CTLR         = 0x00000001; // enable GIC distributor

    //sort pcb in descending order of the priorities
    //sort(pcb);
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
        case 0x01 : {  //write call => write(fd,*x,n)
            int   fd = ( int   )( ctx->gpr[ 0 ] );
            char*  x = ( char* )( ctx->gpr[ 1 ] );
            int    n = ( int   )( ctx->gpr[ 2 ] );
            for( int i = 0; i < n; i++ ) {
                PL011_putc( UART0, *x++, true );
            }
            ctx->gpr[ 0 ] = n;
            break;
        }
        case 0x04 : {  //exit call
            terminate_process();
            schedule_priority(ctx);
            break;
        }
        default : { //case 0x0?
            break;
        }
    }
    return;
}
