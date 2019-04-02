#ifndef __GRAPHICS_H
#define  __GRAPHICS_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

extern void drawLine(uint16_t grid[ 600 ][ 800 ],int x1,int y1,int x2, int y2,uint16_t rgb);
#endif
