#include "game.h"
#include "common.h"
#include "simulation.h"
#include "tui.h"
#include <math.h>
#include <stdio.h>

Game game = { 0 };

bool init() {
    game.width = WINDOW_WIDTH;
    game.height = WINDOW_HEIGHT;
    game.tui.brush_size = 3;
    game.tui.selected_idx = 0;
    game.current_type = PARTICLE_SAND;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return false;
    }

    game.window = SDL_CreateWindow(
        "Falling Sand",
        game.width,
        game.height,
        0
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

    // if (!SDL_SetWindowOpacity(game.window, 0.8f)) {
    //     SDL_Log("Transparent Window: %s", SDL_GetError());
    //     return false;
    // }

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

    if (!text_init(&game.text, game.renderer, "/usr/share/fonts/TTF/Hack-Regular.ttf", 16)) {
        return false;
    }

    if (!tui_init()) {
        return false;
    }

    game.last_time = SDL_GetTicks();
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
            Particle *p = &game.sim.grid[y * SIM_WIDTH + x];

            if (p->active) {
                row[x] = p->render_color;
            } else {
                row[x] = 0x00000000;
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
                game.tui.brush_size += (int)event.wheel.y;
                if (game.tui.brush_size < 1) game.tui.brush_size = 1;
                if (game.tui.brush_size > 20) game.tui.brush_size = 20;
                break;
        }
    }
}

void handle_input() {
    int sim_x, sim_y;
    screen_to_sim(game.mouse_x, game.mouse_y, &sim_x, &sim_y);
    game.current_type = tui_get_selected_type(&game.tui);

    if (game.mouse_left) {
        sim_brush_cirlce(&game.sim, sim_x, sim_y, game.tui.brush_size, game.current_type);
    }

    if (game.mouse_right) {
        sim_brush_erase(&game.sim, sim_x, sim_y, game.tui.brush_size);
    }
}

void update() {
    tui_update(&game.tui);
    if (game.tui.quit_requested) {
        game.running = false;
    }
    if (game.tui.clear_requested) {
        sim_clear(&game.sim);
        game.tui.clear_requested = false;
    }

    handle_input();

    if (!game.tui.paused) {
        sim_update(&game.sim);
    }

    update_texture();

    game.frame_count++;
    Uint64 now = SDL_GetTicks();
    if (now - game.last_time >= 1000) {
        game.fps = (float)game.frame_count * 1000.0f / (now - game.last_time);
        game.frame_count = 0;
        game.last_time = now;
    }
}

void draw() {
    SDL_SetRenderDrawColor(game.renderer, 25, 23, 36, 192);
    SDL_RenderClear(game.renderer);

    render_texture();

    char fps_str[32];
    snprintf(fps_str, sizeof(fps_str), "FPS: %.0f", game.fps);
    text_draw(&game.text, fps_str, 10, 10, (SDL_Color){255, 255, 255, 255});

    SDL_RenderPresent(game.renderer);
}

void run() {
    const Uint64 target_ms = 16;

    while (game.running) {
        Uint64 start = SDL_GetTicks();

        handle_events();
        update();
        draw();
        tui_draw(&game.tui);

        Uint64 frame_time = SDL_GetTicks() - start;

        if (frame_time < target_ms)
            SDL_Delay((Uint32)(target_ms - frame_time));
    }
}

void cleanup() {
    tui_cleanup();
    sim_cleanup(&game.sim);
    SDL_DestroyTexture(game.texture);
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    SDL_Quit();
}
