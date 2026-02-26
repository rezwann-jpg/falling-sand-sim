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
#define COLOR_STONE (Color){ 117, 111, 113, 240 }
#define COLOR_WOOD (Color){ 96, 59, 42, 220 }
#define COLOR_FIRE (Color){ 247, 55, 24, 220 }
#define COLOR_FIRE_MIN (Color){ 185, 58, 50, 255 }
#define COLOR_SMOKE (Color){ 126, 126, 126, 255 }
#define COLOR_STEAM (Color){ 230, 232, 229, 255 }

static Color color_lerp(Color a, Color b, float t) {
    Color result;

    result.r = (unsigned int)(a.r + (b.r - a.r) * t);
    result.g = (unsigned int)(a.g + (b.g - a.g) * t);
    result.b = (unsigned int)(a.b + (b.b - a.b) * t);
    result.a = (unsigned int)(a.a + (b.a - a.a) * t);

    return result;
}

#endif
