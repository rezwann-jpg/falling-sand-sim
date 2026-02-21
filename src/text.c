#include "text.h"

bool text_init(TextRenderer *tr, SDL_Renderer *renderer, const char *font_path, int size) {
    if (!TTF_Init()) {
        SDL_Log("TTF init failed: %s", SDL_GetError());
        return false;
    }

    tr->font = TTF_OpenFont(font_path, size);
    if (!tr->font) {
        SDL_Log("Font load failed: %s", SDL_GetError());
        return false;
    }

    tr->renderer = renderer;
    return true;
}

void text_draw(TextRenderer *tr, const char *str, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Blended(tr->font, str, 0, color);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(tr->renderer, surface);
    if (texture) {
        SDL_FRect dest = {(float)x, (float)y, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(tr->renderer, texture, NULL, &dest);
        SDL_DestroyTexture(texture);
    }

    SDL_DestroySurface(surface);
}

void text_cleanup(TextRenderer *tr) {
    if (tr->font) TTF_CloseFont(tr->font);
    TTF_Quit();
}
