#ifndef COLOR_H_
#define COLOR_H_

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

#define COLOR_SAND (Color){ 236, 204, 160, 220 }
#define COLOR_WATER (Color){ 66, 135, 245, 180 }
#define COLOR_AIR (Color){ 0, 0, 0, 0 }

#endif
