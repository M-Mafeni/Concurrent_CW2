/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"
#define waitNo 25
#define WHITE 0x7FFF
#define RED 0x001F
#define GREEN 0x03E0
#define BLUE 0x7C00
#define BLACK 0x000
#define CURSOR_SIZE 15
pcb_t* current = NULL;
pcb_t pcb[50];
int cursorPosition[2] = {DISPLAY_HEIGHT/2,DISPLAY_WIDTH/2};
waitingProcess waiting[waitNo];
//grid used for display
uint16_t grid[ DISPLAY_HEIGHT ][ DISPLAY_WIDTH ];
/*array stores the stack pointers for the processes for
better memory allocation*/
uint32_t topOfProcesses[50];
int length = sizeof(pcb) / sizeof(pcb[0]);
uint32_t topOfStack;

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

extern void     main_console2();
extern uint32_t tos_console;
extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_philosopher();

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

        //put process in queue
        pcb[child.pid] = child;

    }
    return;
}
void exec_program(ctx_t* ctx,uint32_t address){
    ctx->pc = address;
    ctx->cpsr = 0X50;
    dispatch(ctx,current,current);
    return;
}
void kill_process(int id) {
    pcb[id].status = STATUS_TERMINATED;
}
void configDisplay(){
    // Configure the LCD display into 800x600 SVGA @ 36MHz resolution.

    SYSCONF->CLCD      = 0x2CAC;     // per per Table 4.3 of datasheet
    LCD->LCDTiming0    = 0x1313A4C4; // per per Table 4.3 of datasheet
    LCD->LCDTiming1    = 0x0505F657; // per per Table 4.3 of datasheet
    LCD->LCDTiming2    = 0x071F1800; // per per Table 4.3 of datasheet

    LCD->LCDUPBASE     = ( uint32_t )( &grid );

    LCD->LCDControl    = 0x00000020; // select TFT   display type
    LCD->LCDControl   |= 0x00000008; // select 16BPP display mode
    LCD->LCDControl   |= 0x00000800; // power-on LCD controller
    LCD->LCDControl   |= 0x00000001; // enable   LCD controller

        /* Configure the mechanism for interrupt handling by
     *
     * - configuring then enabling PS/2 controllers st. an interrupt is
     *   raised every time a byte is subsequently received,
     * - configuring GIC st. the selected interrupts are forwarded to the
     *   processor via the IRQ interrupt signal, then
     * - enabling IRQ interrupts.
     */
    PS20->CR           = 0x00000010; // enable PS/2    (Rx) interrupt
    PS20->CR          |= 0x00000004; // enable PS/2 (Tx+Rx)
    PS21->CR           = 0x00000010; // enable PS/2    (Rx) interrupt
    PS21->CR          |= 0x00000004; // enable PS/2 (Tx+Rx)

    uint8_t ack;

          PL050_putc( PS20, 0xF4 );  // transmit PS/2 enable command
    ack = PL050_getc( PS20       );  // receive  PS/2 acknowledgement
          PL050_putc( PS21, 0xF4 );  // transmit PS/2 enable command
    ack = PL050_getc( PS21       );  // receive  PS/2 acknowledgement

    GICC0->PMR         = 0x000000F0; // unmask all          interrupts
    GICD0->ISENABLER1 |= 0x00300000; // enable PS2          interrupts
    GICC0->CTLR        = 0x00000001; // enable GIC interface
    GICD0->CTLR        = 0x00000001; // enable GIC distributor
}
int colorMap[CURSOR_SIZE][CURSOR_SIZE];
//shows the state of the programs executed from GUI
unsigned char state = 0x0000; //programs start off not running
void drawState(){
    int shift = 0;
    char* string;
    for(int i = 3; i >= 0; i--){
        int progState = (state >> i) & 0x1;
        string = (progState) ? "RUNNING" : "NOT RUNNING";
        int colour = (progState) ? GREEN : RED;
        //reset state that was there before
        drawRectangle(grid,460,40 + shift,20,100,BLUE);
        drawString(grid,string,460,60 + shift,1,colour);
        shift += 220;
    }
}
int logoColours[4] = {WHITE,RED,GREEN,BLUE};
int logoIndex = 0;
void drawLogo() {
    drawRectangle(grid,0,250,80,240,BLACK);
    //logo region = (0,250) ->(80,590)
    drawString(grid,"QEMU",0,250,10,logoColours[logoIndex]);
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
        pcb[i].waitingTime = 0;
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
    console.ctx.pc   = ( uint32_t )( &main_console2 );
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
    configDisplay();
    memset(&colorMap,0,sizeof(colorMap) * sizeof(colorMap[0]));
    drawLogo();
    drawString(grid,"PRESS A TO RUN P3",120,250,2,RED);
    drawString(grid,"PRESS S TO RUN P4",150,250,2,RED);
    drawString(grid,"PRESS D TO RUN P5",180,250,2,RED);
    drawString(grid,"PRESS F TO RUN PHILOSOPHER",210,250,2,RED);
    drawString(grid,"PRESS K TO KILL ALL PROGRAMS",240,250,2,RED);
    //create buttons
    drawRectangle(grid,400,30,80,140,BLUE);
    drawRectangle(grid,400,250,80,140,BLUE);
    drawRectangle(grid,400,470,80,140,BLUE);
    drawRectangle(grid,400,690,80,140,BLUE);
    drawString(grid,"P3",420,60,5,BLACK);
    drawString(grid,"P4",420,280,5,BLACK);
    drawString(grid,"P5",420,500,5,BLACK);
    drawString(grid,"PHILOSOPHER",420,720,1,BLACK);
    drawState();
    //initialise cursor
    drawSquare(grid,cursorPosition[0],cursorPosition[1],CURSOR_SIZE,WHITE);
    dispatch( ctx, NULL, &pcb[0] );
    int_enable_irq();
    return;
}

//checks through waiting queue to see if semaphore value changed
void checkAvailable(){
    for(int i = 0; i < waitNo; i++){
        waitingProcess w = waiting[i];
        pcb[w.pid].priority += pcb[w.pid].priority_change * 2;
        if((w.semaphore) != NULL && *(w.semaphore) == 0){
            if(!is_terminated(pcb[w.pid])){
                pcb[w.pid].status = STATUS_READY;
                //remove from queue
                w.pid = -1;
                w.semaphore = NULL;
            }
        }
    }
}
//checks through process queue for processes
//that were put to sleep and decreases their time
void awaken(){
    for(int i = 0; i < length; i++){
        if(pcb[i].waitingTime > 0){
            pcb[i].waitingTime -= 1;
            pcb[i].priority += pcb[i].priority_change * 2;
            if(pcb[i].waitingTime == 0 && !is_terminated(pcb[i])){
                pcb[i].status = STATUS_READY;
            }
        }
    }
}
//moves mouse based on offset values
void moveMouse(int xOffset, int yOffset){
    int prevPosition[2];
    memcpy(&prevPosition,&cursorPosition,sizeof(cursorPosition));
    //replace square with colours that were there before
    for(int i = 0; i < CURSOR_SIZE;i++ ){
        for(int j = 0; j < CURSOR_SIZE; j++){
             grid[cursorPosition[0] + i][cursorPosition[1] + j] = colorMap[i][j];
        }
    }
    // update position
    if(cursorPosition[0] + xOffset >= 0 && cursorPosition[0] + xOffset <= DISPLAY_HEIGHT - CURSOR_SIZE){ //in x range
        cursorPosition[0] += xOffset;
    }
    if(cursorPosition[1] + yOffset >= 0 && cursorPosition[1] + yOffset <= DISPLAY_WIDTH - CURSOR_SIZE){ //in y range
        cursorPosition[1] += yOffset;
    }    //create square at new position
    //preserve color of location about to be travelled
    for(int i = 0; i < CURSOR_SIZE;i++ ){
        for(int j = 0; j < CURSOR_SIZE; j++){
            colorMap[i][j] = grid[cursorPosition[0] + i][cursorPosition[1] + j];
        }
    }
    drawSquare(grid,cursorPosition[0],cursorPosition[1],CURSOR_SIZE,WHITE);
}

void addProcessFromGUI(uint32_t address,ctx_t*ctx){
    int id = getUniqueId();
    create_new_process(ctx);
    pcb[id].ctx.pc = address;
}
void kill_all_programs(){
    int i = 1;
    while(pcb[i].pid != -1){
        pcb[i].status = STATUS_TERMINATED;
        i++;
    }
}
bool bytesReceived[3] = {false,false,false};
signed char bytes[3];
void hilevel_handler_irq(ctx_t* ctx) {
    // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.
  if ( id == GIC_SOURCE_TIMER0 ) {
      awaken();schedule_priority(ctx); TIMER0->Timer1IntClr = 0x01;
    }
  else if ( id == GIC_SOURCE_PS20 ) { //keyboard interrupt
   uint8_t x = PL050_getc( PS20 );
   // char scanCode = ( x >> 4 ) & 0xF;
   char scanCode = x;
   char key = decodeKeyPress(scanCode);
   // PL011_putc(UART0,key,true);
   switch(key){
       case 'A':{ //execute P3
           addProcessFromGUI((uint32_t) &main_P3,ctx);
           state = state | 0x8;
           break;
       }
       case 'S':{ //execute P4
           addProcessFromGUI((uint32_t) &main_P4,ctx);
           state = state | 0x4;
           break;
       }
       case 'D':{ //execute P5
           addProcessFromGUI((uint32_t) &main_P5,ctx);
           state = state | 0x2;
           break;
       }
       case 'F':{ //execute philosopher
           addProcessFromGUI((uint32_t) &main_philosopher,ctx);
           state = state | 0x1;
           break;
       }
       case 'K':{//kill all programs
           kill_all_programs();
           state = 0;
           break;
       }
       default:{
           break;
       }
   }
   drawState();
 }
 else if( id == GIC_SOURCE_PS21 ) { //mouse interrupt
   signed char x = PL050_getc( PS21 );
   if(!bytesReceived[0]){ //1st byte has not been received
       bytesReceived[0] = true;
       bytes[0] = x;
       //logo region = (0,250) ->(80,590)
       unsigned char buttonClicked = bytes[0] & 0x1;
       if(buttonClicked){
           bool inXRegion = (cursorPosition[0] <= 80);
           bool inYRegion = (cursorPosition[1] >= 250 && cursorPosition[1] <= 590);
           if(inXRegion && inYRegion){
               logoIndex = (logoIndex + 1) % 4;
               drawLogo();
           }
           bytes[0] = 0;
       }
   }else if(!bytesReceived[1]){ //x byte has not been received
       bytesReceived[1] = true;
       bytes[1] = x;
   }else{ //y byte has not been received
       bytesReceived[2] = true;
       bytes[2] = x;
       signed char xOffset = bytes[1];
       signed char yOffset = bytes[2];
       moveMouse(yOffset,xOffset);
       //reset the bytesReceived values
       for(int i = 0; i < 3; i++){
           bytesReceived[i] = false;
       }
       //move mouse here
   }
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
            int child = getUniqueId();
            create_new_process(ctx);
            pcb[child].ctx.gpr[0] = 0;
            ctx->gpr[0] = child;
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
        case 0x07 : { //Nice Call
            pid_t pid = (pid_t)(ctx->gpr[0]);
            int priority = (int) (ctx->gpr[1]);
            pcb[pid].priority = priority;
        }
        case 0x08 : { //sem init
            break;
        }
        case 0x09:{ //sem wait
            //this should stop execution until semaphore value is available i.e > 0
            sem_t* val = (sem_t*)(ctx->gpr[0]);
            if(*val <= 0){ // not available
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
                (*val)--;
            }
            break;
        }
        case 0x10:{ //sem post
            sem_t* val = (sem_t*)(ctx->gpr[0]);
            (*val)++; //resource is now available for use
            checkAvailable();
            break;
        }
        case 0x0A:{ //sem destroy
            break;
        }
        case 0x0B:{ //return PID
            ctx->gpr[0] = current->pid;
            break;
        }
        case 0X0C:{ //sleep
            int time = ctx->gpr[0];
            current->waitingTime = time;
            current->status = STATUS_WAITING;
            schedule_priority(ctx);
            break;
        }
        default : { //case 0x0?
            break;
        }
    }
    return;
}
