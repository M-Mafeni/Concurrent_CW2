#ifndef __GRAPHICS_H
#define  __GRAPHICS_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

extern void drawLine(uint16_t grid[ 600 ][ 800 ],int x1,int y1,int x2, int y2,uint16_t rgb);
//draw a circle with centre (x, y) giving r as its radius
extern void drawCircle(uint16_t grid[ 600 ][ 800 ],int x,int y,int r);
//draw a square with top left corner (x ,y) with length l
extern void drawSquare(uint16_t grid[ 600 ][ 800 ],int x,int y,int l);
extern void drawBox(uint16_t grid[ 600 ][ 800 ],int x,int y,int l);
extern void drawChar(uint16_t grid[ 600 ][ 800 ],unsigned char c,int xOffset,int yOffset,uint16_t rgb);

#endif
