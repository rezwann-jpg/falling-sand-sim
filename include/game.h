#ifndef GAME_H_
#define GAME_H_

#include <SDL3/SDL.h>
#include "simulation.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    bool running;
    int width;
    int height;

    Simulation sim;
} Game;

extern Game game;

bool init();
void run();
void cleanup();

void update_texture();
void render_texture();

#endif
