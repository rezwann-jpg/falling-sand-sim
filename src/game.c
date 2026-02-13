#include "game.h"
#include "common.h"
#include "particle.h"
#include "simulation.h"
#include <SDL3/SDL_events.h>
#include <math.h>
#include <stdio.h>

Game game = { 0 };

bool init() {
    game.width = WINDOW_WIDTH;
    game.height = WINDOW_HEIGHT;
    game.brush_size = 3;
    game.current_type = PARTICLE_SAND;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return false;
    }

    game.window = SDL_CreateWindow(
        "Falling Sand",
        game.width,
        game.height,
        SDL_WINDOW_TRANSPARENT
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

    if (!sim_init(&game.sim)) {
        fprintf(stderr, "Simulation init error\n");
        return false;
    }

    game.running = true;

    return true;
}

void screen_to_sim(int screen_x, int screen_y, int *sim_x, int *sim_y) {
    float scale = fminf(
        (float)game.width / SIM_WIDTH,
        (float)game.height / SIM_HEIGHT
    );

    float render_w = SIM_WIDTH * scale;
    float render_h = SIM_HEIGHT * scale;
    float offset_x = (game.width - render_w) / 2;
    float offset_y = (game.height - render_h) / 2;

    *sim_x = (int)((screen_x - offset_x) / scale);
    *sim_y = (int)((screen_y - offset_y) / scale);
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
        Uint32 *row = pixel_buffer + y * row_pixels;
        for (int x = 0; x < SIM_WIDTH; x++) {
            Particle *p = game.sim.grid[y * SIM_WIDTH + x];

            if (p) {
                row[x] = (p->color.a << 24) |
                         (p->color.b << 16) |
                         (p->color.g << 8) |
                         (p->color.r);
            }
        }
    }

    SDL_UnlockTexture(game.texture);
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


            case SDL_EVENT_MOUSE_MOTION:
                game.mouse_x = (int)event.motion.x;
                game.mouse_y = (int)event.motion.y;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                    game.mouse_left = true;
                if (event.button.button == SDL_BUTTON_RIGHT)
                    game.mouse_right = true;
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_LEFT)
                    game.mouse_left = false;
                if (event.button.button == SDL_BUTTON_RIGHT)
                    game.mouse_right = false;
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                game.brush_size += (int)event.wheel.y;
                if (game.brush_size < 1) game.brush_size = 1;
                if (game.brush_size > 20) game.brush_size = 20;
                break;

            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key) {
                    case SDLK_1:
                        game.current_type = PARTICLE_SAND;
                        break;
                    case SDLK_2:
                        game.current_type = PARTICLE_WATER;
                        break;
                    case SDLK_C:
                        for (int i = 0; i < SIM_WIDTH * SIM_HEIGHT; i++) {
                            if (game.sim.grid[i]) {
                                int y = i / SIM_WIDTH;
                                int x = i % SIM_WIDTH;
                                sim_remove_particle(&game.sim, x, y);
                            }
                        }
                        break;
                }
                break;
        }
    }
}

void handle_input() {
    int sim_x, sim_y;
    screen_to_sim(game.mouse_x, game.mouse_y, &sim_x, &sim_y);

    if (game.mouse_left) {
        sim_brush_cirlce(&game.sim, sim_x, sim_y, game.brush_size, game.current_type);
    }

    if (game.mouse_right) {
        sim_brush_erase(&game.sim, sim_x, sim_y, game.brush_size);
    }
}

void update() {
    handle_input();
    sim_update(&game.sim);
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
    sim_cleanup(&game.sim);
    SDL_DestroyTexture(game.texture);
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    SDL_Quit();
}
