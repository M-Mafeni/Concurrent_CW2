# Implementation of an Operating System
This project implements a simple OS on a RealView Platform Baseboard for Cortex-A8 emulated by QEMU. It contains a terminal and supports some user processes (such as fork and exec) as defined by POSIX. It also hosts a simple user interface.
## Running it
To run it on a linux machine:
1. first, install qemu via sudo apt-get install qemu
2. cd into the question folder and open 3 terminals
3. in one terminal execute the command "make launch-qemu"
4. In another execute "make launch-console". this would be where the emulated terminal is
5. in the last one execute "make launch-gdb". this is the debugging terminal
6. in the debugging terminal, type in continue to use it



