/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */
#include "hilevel.h"
#define waitNo 25
pcb_t* current = NULL;
pcb_t pcb[50];
waitingProcess waiting[waitNo];
/*array stores the stack pointers for the processes for
better memory allocation*/
uint32_t topOfProcesses[50];
int length = sizeof(pcb) / sizeof(pcb[0]);
uint32_t topOfStack;

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


//checks whether a process has been terminated
int is_terminated(pcb_t process){
    return process.status == STATUS_TERMINATED;
}

//returns the index of the pcb block with the highest priority
// that's not terminated
int getMax(){
    int max_priority = -1;
    int max_index = -1;
    for( int i = 0; i < length; i++){
        if(pcb[i].priority > max_priority && !(pcb[i].status == STATUS_TERMINATED || pcb[i].status == STATUS_WAITING) && pcb[i].pid != -1){
            max_priority = pcb[i].priority;
            max_index = i;
        }
    }
    return max_index;
}
void schedule_priority(ctx_t* ctx){
    int max = getMax();
    dispatch(ctx,current,&pcb[max]);
    pcb[max].status = STATUS_EXECUTING;
    for(int i = 0; i < length; i++){
        if(pcb[i].pid != -1 && !(pcb[i].status == STATUS_TERMINATED || pcb[i].status == STATUS_WAITING) && i != max){
            pcb[i].status = STATUS_READY;
            pcb[i].priority += pcb[i].priority_change;
        }
    }
    return;
}

int getUniqueId(){
    for(int i = 0; i < length; i++){
        if(pcb[i].pid == -1 || is_terminated(pcb[i])){
            return i;
        }
    }
    return -1;
}

extern void     main_console();
extern uint32_t tos_console;

void create_new_process(ctx_t* ctx){
    int id = getUniqueId();
    if(id == -1){
        ctx->gpr[0] = -1;
    }else{
        pcb_t child;
        memset(&child, 0, sizeof(pcb_t));
        child.pid = id;
        child.status = current->status;
        child.ctx.cpsr = ctx->cpsr;
        child.ctx.pc = ctx->pc;
        child.priority = current->priority;
        child.priority_change = current->priority_change;
        memcpy(child.ctx.gpr,ctx->gpr,sizeof(child.ctx.gpr));
        uint32_t topOfNewProcess = topOfProcesses[id];
        if(topOfNewProcess == 0){
            topOfNewProcess = topOfStack + 0x00001000;
        }
        topOfStack = topOfNewProcess;
        topOfProcesses[id] = topOfStack;
        uint32_t offset = ctx->sp - current->ctx.sp;
        //problem with retaining local variables
        memcpy((void*)(topOfProcesses[id]- offset),(void*)(topOfProcesses[current->pid] - offset ),offset);
        child.ctx.sp = topOfNewProcess - offset;
        child.ctx.lr = ctx->lr;
        //put in return values
        child.ctx.gpr[0] = 0;
        ctx->gpr[0] = child.pid;
        //put process in queue
        pcb[child.pid] = child;
  
    }
    return;
}
void exec_program(ctx_t* ctx,uint32_t address){
    ctx->pc = address;
    dispatch(ctx,current,current);
    return;
}
void kill_process(int id) {
    pcb[id].status = STATUS_TERMINATED;
}



void hilevel_handler_rst( ctx_t* ctx              ) {
    /* Initialises PCBs, representing user processes stemming from execution
    * of user programs.  Note in each case that
    *
    * - the CPSR value of 0x50 means the processor is switched into USR mode,
    *   with IRQ interrupts enabled, and
    * - the PC and SP values match the entry point and top of stack.
    */
    PL011_putc(UART0,'R',true);
    //initialise process block with every process having id -1
    for(int i = 0; i < length; i++){
        pcb[i].pid = -1;
        topOfProcesses[i] = 0;
    }
    for(int i = 0; i < waitNo; i++){
        waiting[i].pid = -1;
        waiting[i].semaphore = NULL;
    }
    pcb_t console;
    memset(&console, 0, sizeof(pcb_t));
    console.pid = 0;
    console.status   = STATUS_CREATED;
    console.ctx.cpsr = 0x50;
    console.ctx.pc   = ( uint32_t )( &main_console );
    console.ctx.sp   = ( uint32_t )( &tos_console  );
    console.priority_change = 1;
    console.priority = 30;
    pcb[0]= console;
    topOfStack = console.ctx.sp;
    topOfProcesses[0] = console.ctx.sp;

    TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
    TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
    TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
    TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
    TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

    GICC0->PMR          = 0x000000F0; // unmask all            interrupts
    GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
    GICC0->CTLR         = 0x00000001; // enable GIC interface
    GICD0->CTLR         = 0x00000001; // enable GIC distributor

    int max = getMax();
    dispatch( ctx, NULL, &pcb[0] );
    int_enable_irq();
    return;
}

//checks through waiting queue to see if semaphore value changed
void checkAvailable(){
    for(int i = 0; i < waitNo; i++){
        waitingProcess w = waiting[i];
        if((w.semaphore) != NULL && *(w.semaphore) == 0){
            pcb[w.pid].status = STATUS_READY;
            //remove from queue
            w.pid = -1;
            w.semaphore = NULL;
        }
    }
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
        case 0x00: { //yield call
            schedule_priority(ctx);
            break;
        }
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
        case 0x03 : { //fork call
            PL011_putc(UART0, 'F', true);
            create_new_process(ctx);
            break;
        }
        case 0x04 : {  //exit call
            PL011_putc(UART0,'Q',true);
            current->status = STATUS_TERMINATED;
            schedule_priority(ctx);
            break;
        }
        case 0x05 : { //exec call
            PL011_putc(UART0, 'E', true);
            uint32_t address = (uint32_t)(ctx->gpr[0]);
            exec_program(ctx,address);
            break;
        }
        case 0x06 : { //kill call
            PL011_putc(UART0, 'K', true);
            int id = (int)(ctx->gpr[0]);
            kill_process(id);
            break;
        }
        case 0x08 : { //sem init
            break;
        }
        case 0x09:{ //sem wait
            //this should stop execution until semaphore values is availble
            //i.e = 0
            sem_t* val = (sem_t*)(ctx->gpr[0]);
            if(*val == 1){ //not available
                dispatch(ctx,current,current);
                current->status = STATUS_WAITING;
                //place entry in waiting queue
                for(int i= 0; i < waitNo; i++){
                    if(waiting[i].pid == -1){
                        waiting[i].pid = current->pid;
                        waiting[i].semaphore = val;
                        schedule_priority(ctx);
                        break;
                    }
                }
            }else{
                *val = 1; //resource is now in use
            }
            break;         
        }
        case 0x10:{ //sem post
            sem_t* val = (sem_t*)(ctx->gpr[0]);
            *val = 0; //resource is now available for use
            checkAvailable();
            schedule_priority(ctx);
            break;
        }
        case 0x0A:{ //sem destroy
            break;
        }
        case 0x0B:{ //return PID
            ctx->gpr[0] = current->pid;
        }
        default : { //case 0x0?
            break;
        }
    }
    return;
}
