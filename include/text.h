#ifndef TEXT_H_
#define TEXT_H_

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

typedef struct {
    TTF_Font *font;
    SDL_Renderer *renderer;
}TextRenderer;

bool text_init(TextRenderer *tr, SDL_Renderer *renderer, const char *font_path, int size);
void text_draw(TextRenderer *tr, const char *str, int x, int y, SDL_Color color);
void text_cleanup(TextRenderer *tr);

#endif
