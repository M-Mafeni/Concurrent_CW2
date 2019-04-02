#include "graphics.h"
void drawLine(uint16_t grid[ 600 ][ 800 ],int x1,int y1,int x2, int y2,uint16_t rgb){
    double m =  ( (double) y2- y1)/((double) x2 - x1);
    int c = y2 - m * x2;
    for(int x = x1; x <= x2; x++){
        int y = m * x + c;
        for(int j = x - 10; j <= x + 10; j++){
            grid[j][y - 1] = rgb;
            grid[j][y] = rgb;
            grid[j][y + 1] = rgb;
        }
    }
}
