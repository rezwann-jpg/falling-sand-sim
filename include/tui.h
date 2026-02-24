#ifndef TUI_H_
#define TUI_H_

#include "particle.h"
#include <stdbool.h>

typedef struct {
    int selected_idx;
    int brush_size;
    bool paused;
    bool clear_requested;
    bool quit_requested;
} TuiState;

bool tui_init(void);
void tui_cleanup(void);
void tui_update(TuiState *state);
void tui_draw(TuiState *state);
ParticleType tui_get_selected_type(TuiState *state);

#endif
