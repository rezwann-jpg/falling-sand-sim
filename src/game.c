#include "game.h"
#include "common.h"
#include <math.h>

bool init() {
    game.width = WINDOW_WIDTH;
    game.height = WINDOW_HEIGHT;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return false;
    }

    game.window = SDL_CreateWindow(
        "Falling Sand",
        game.width,
        game.height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT
    );

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

    game.texture =  SDL_CreateTexture(
        game.renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        SIM_WIDTH,
        SIM_HEIGHT
    );

    if (!game.texture) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetTextureBlendMode(game.texture, SDL_BLENDMODE_BLEND)) {
        SDL_Log("Blend mode setup failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_SetRenderDrawBlendMode(game.renderer, SDL_BLENDMODE_BLEND)) {
        SDL_Log("Renderer blend mode setup failed: %s", SDL_GetError());
        SDL_DestroyRenderer(game.renderer);
        SDL_DestroyWindow(game.window);
        SDL_Quit();
        return false;
    }

    game.running = true;

    return true;
}

void update_texture() {
    void *pixels;
    int pitch;

    if (!SDL_LockTexture(game.texture, NULL, &pixels, &pitch)) {
        SDL_Log("Failed to lock texture: %s", SDL_GetError());
        return;
    }

    Uint32 *pixel_buffer = (Uint32 *)pixels;
    int row_pixels = pitch / sizeof(Uint32);

    for (int y = 0; y < SIM_HEIGHT; y++) {
        Uint32 *row = pixel_buffer + y * row_pixels;
        for (int x = 0; x < SIM_WIDTH; x++) {
            row[x] = 0x00000000;
        }
    }

    for (int y = 0; y < SIM_HEIGHT; y++) {
        for (int x = 0; x < SIM_WIDTH; x++) {

        }
    }
}

void render_texture() {
    float scale = fminf(
        (float)game.width / SIM_WIDTH,
        (float)game.height / SIM_HEIGHT
    );

    float render_w = SIM_WIDTH * scale;
    float render_h = SIM_HEIGHT * scale;

    SDL_FRect dest = {
        (game.width - render_w) / 2,
        (game.height - render_h) / 2,
        render_w,
        render_h
    };

    SDL_RenderTexture(
        game.renderer,
        game.texture,
        NULL,
        &dest
    );
}

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                game.running = false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                game.width = event.window.data1;
                game.height = event.window.data2;
                break;
        }
    }
}

void update() {
    update_texture();
}

void draw() {
    SDL_SetRenderDrawColor(game.renderer, 25, 23, 36, 192);
    SDL_RenderClear(game.renderer);

    render_texture();

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
    SDL_DestroyTexture(game.texture);
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    SDL_Quit();
}
