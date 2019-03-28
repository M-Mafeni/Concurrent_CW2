/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */
#define No_of_pipes 40
#include "hilevel.h"
pcb_t* current = NULL;
pcb_t pcb[50];
/*array stores the stack pointers for the processes for
better memory allocation*/
uint32_t topOfProcesses[50];
pipe pipes[No_of_pipes];
int length = sizeof(pcb) / sizeof(pcb[0]);
int activePipes = 0;
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
void place_on_pipe(ctx_t* ctx,uint32_t sourceId,uint32_t destId,void* data){
    for(int i = 0; i < No_of_pipes; i++){
        if(pipes[i].sourceId == sourceId && pipes[i].destId == destId){
            if(pipes[i].data == NULL){
                pipes[i].data = data;
                data = NULL;
                break;
            }else{
                //place in buffer
               for(int j = 0; j < 3; j++){
                   if(pipes[i].buffer[j] == NULL){
                       pipes[i].buffer[j] = data;
                       break;
                   } 
               }
               //currently doesn't deal with the case when the buffer's full
            }
        }
    }
    return;
}
void receive_from_pipe(ctx_t* ctx,uint32_t destId,uint32_t sourceId){
    for(int i = 0; i < No_of_pipes; i++){
        pipe p = pipes[i];
        if(p.destId == destId && p.sourceId == sourceId){
             if(pipes[i].data != NULL){
                void* data = pipes[i].data;
                ctx->gpr[0] = (uint32_t) data;
                pipes[i].data = NULL;
                return;
             }else{   
                 //check buffer
                for(int j = 0; j < 3; j++){
                    if(pipes[i].buffer[j] != NULL){
                        void* data = pipes[i].buffer[j];
                        ctx->gpr[0] = (uint32_t) data;
                        pipes[i].buffer[j] = NULL;
                        return;
                    }
                }
                //if no data was found
                current->status = STATUS_WAITING;
                pipes[i].waitingToReceive = true;
                dispatch(ctx,current,current);
                schedule_priority(ctx);
                break;
             }
        }
    }
}

void checkAvailableToReceive(){
    for(int i = 0; i < No_of_pipes; i++){
        pipe p = pipes[i];
        if(p.waitingToReceive){
            //check for data
            pid_t processId = p.processId;
            if(p.data != NULL){
                pcb[processId].ctx.gpr[0] = (uint32_t) p.data;
                pcb[processId].status = STATUS_READY;
                p.waitingToReceive = false;
                break;
            }else{
                //check buffer
                for(int j = 0; j < 3; j++){
                    if(p.buffer[j] != NULL){
                        pcb[processId].ctx.gpr[0] = (uint32_t) p.buffer[j];
                        pcb[processId].status = STATUS_READY;
                        p.waitingToReceive = false;
                        break;
                    }
                }
                //no data was found. increase priority
                pcb[processId].priority += pcb[processId].priority_change;
            }
        }
    } 
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
        if(i < 16){
            //set default pipe values
            pipes[i].data = NULL;
        }
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
/*Should loop through all pipes and check whether the data is/isn't available
 */


void hilevel_handler_irq(ctx_t* ctx) {
    // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_TIMER0 ) {
    checkAvailableToReceive();schedule_priority(ctx); TIMER0->Timer1IntClr = 0x01;
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
        case 0x08 : { //create new pipe
            PL011_putc(UART0,'P',true);
            int* fd = (int*) ctx->gpr[0];
            pipes[activePipes].processId = current->pid;
            pipes[activePipes].sourceId = (uint32_t)*fd;
            pipes[activePipes].destId   = (uint32_t)*(fd + 1);
            pipes[activePipes].data     = NULL;
            pipes[activePipes].waitingToSend = false;
            pipes[activePipes].waitingToReceive = false;
            activePipes++;
            break;
        }
        case 0x09:{ //send from source to dest
            PL011_putc(UART0,'S',true);
            uint32_t sourceId = (uint32_t) (ctx->gpr[0]);
            uint32_t destId  =  (uint32_t) (ctx->gpr[1]);
            void* data        = (void*) (ctx->gpr[2]);
            place_on_pipe(ctx,sourceId,destId,data);
            break;
        }
        case 0x10:{ //receive from dest 
            PL011_putc(UART0,'R',true);
            PL011_putc(UART0,'E',true);
            PL011_putc(UART0,'C',true);
            uint32_t destId = (uint32_t) (ctx->gpr[0]);
            uint32_t sourceId = (uint32_t) (ctx->gpr[1]);
            receive_from_pipe(ctx,destId,sourceId);
            break;
        }
        case 0x0A:{ //return PID
            ctx->gpr[0] = current->pid;
        }
        default : { //case 0x0?
            break;
        }
    }
    return;
}
