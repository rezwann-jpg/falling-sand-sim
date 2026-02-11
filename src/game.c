#include "game.h"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

Game game = { 0 };

bool init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return false;
    }

    game.window = SDL_CreateWindow("Falling Sand", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT);
    if (!game.window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    game.renderer = SDL_CreateRenderer(game.window, NULL);
    if (!game.renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(game.window);
        SDL_Quit();
        return false;
    }

    if (!SDL_SetWindowOpacity(game.window, 0.8f)) {
        SDL_Log("Transparent Window: %s", SDL_GetError());
        return false;
    }

    game.running = true;

    return true;
}

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                game.running = false;
                break;
        }
    }
}

void update() {

}

void draw() {
    SDL_SetRenderDrawColor(game.renderer, 25, 23, 36, 192);
    SDL_RenderClear(game.renderer);

    SDL_RenderPresent(game.renderer);
}

void run() {
    Uint64 last_time = SDL_GetTicks();
    const Uint64 FRAME_DELAY = 16;

    while (game.running) {
        Uint64 current_time = SDL_GetTicks();
        Uint64 elapsed = current_time - last_time;

        handle_events();

        if (elapsed >= FRAME_DELAY) {
            update();
            last_time = current_time;
        }

        draw();

        Uint64 frame_time = SDL_GetTicks() - current_time;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay((Uint32)(FRAME_DELAY - frame_time));
        }
    }
}

void cleanup() {
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    SDL_Quit();
}
