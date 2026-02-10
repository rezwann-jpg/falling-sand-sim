#ifndef GAME_H_
#define GAME_H_

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
} Game;

bool init();
void run();

#endif
