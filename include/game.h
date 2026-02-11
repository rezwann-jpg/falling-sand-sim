#ifndef GAME_H_
#define GAME_H_

#include <SDL3/SDL.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    bool running;
    int width;
    int height;
} Game;

bool init();
void run();
void cleanup();
void draw();
void update();
void handle_events();

void update_texture();
void render_texture();

#endif
