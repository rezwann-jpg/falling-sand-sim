#ifndef GAME_H_
#define GAME_H_

#include <SDL3/SDL.h>
#include "particle.h"
#include "simulation.h"
#include "text.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    TextRenderer text;

    bool running;
    int width;
    int height;

    Simulation sim;

    int mouse_x;
    int mouse_y;
    bool mouse_left;
    bool mouse_right;
    int brush_size;
    ParticleType current_type;

    float fps;
    Uint32 last_time;
    int frame_count;
} Game;

extern Game game;

bool init();
void run();
void cleanup();

#endif
