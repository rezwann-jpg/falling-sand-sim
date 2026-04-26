#include "tui.h"
#include <ncurses.h>

static WINDOW *win;

static const struct {
    ParticleType type;
    const char *name;
    int color;
} particles[] = {
    {PARTICLE_SAND, "Sand", COLOR_YELLOW},
    {PARTICLE_WATER, "Water", COLOR_BLUE},
    {PARTICLE_STONE, "Stone", COLOR_WHITE},
    {PARTICLE_WOOD, "Wood", COLOR_YELLOW},
    {PARTICLE_FIRE, "Fire", COLOR_RED},
    {PARTICLE_PLANT, "Plant", COLOR_GREEN},
    {PARTICLE_SEED, "Seed", COLOR_YELLOW},
    {PARTICLE_WET_SAND, "Wet Sand", COLOR_YELLOW},
    {PARTICLE_LAVA, "Lava", COLOR_RED},
    {PARTICLE_ASH, "Ash", COLOR_WHITE},
    {PARTICLE_ANT, "Ant", COLOR_CYAN},
    {PARTICLE_FLOWER, "Flower", COLOR_MAGENTA},
    {PARTICLE_SPORE, "Spore", COLOR_YELLOW},
    {PARTICLE_SPORELING, "Sporeling", COLOR_CYAN},
    {PARTICLE_DEBRIS, "Debris", COLOR_WHITE},
    {PARTICLE_DPL_A, "Life Red", COLOR_RED},
    {PARTICLE_DPL_B, "Life Green", COLOR_GREEN},
    {PARTICLE_DPL_C, "Life Blue", COLOR_BLUE},
    {PARTICLE_DPL_SPAWNER, "DPL Spawner", COLOR_WHITE},
};

#define NUM_PARTICLES ((int)(sizeof(particles) / sizeof(particles[0])))

bool tui_init(void) {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_YELLOW, -1);
        init_pair(2, COLOR_BLUE, -1);
        init_pair(3, COLOR_WHITE, -1);
        init_pair(4, COLOR_RED, -1);
        init_pair(5, COLOR_CYAN, -1);
        init_pair(6, COLOR_GREEN, -1);
        init_pair(7, COLOR_MAGENTA, -1);
    }

    win = newwin(25, 26, 0, 0);
    keypad(win, TRUE);
    nodelay(win, TRUE);

    return true;
}

void tui_cleanup(void) {
    delwin(win);
    endwin();
}

static int color_to_pair(int color) {
    switch (color) {
        case COLOR_YELLOW: return 1;
        case COLOR_BLUE:   return 2;
        case COLOR_WHITE:  return 3;
        case COLOR_RED:    return 4;
        case COLOR_CYAN:   return 5;
        case COLOR_GREEN:  return 6;
        case COLOR_MAGENTA:return 7;
        default:           return 3;
    }
}

void tui_update(TuiState *state) {
    int ch = wgetch(win);
    if (ch == ERR) return;

    switch (ch) {
        case KEY_UP:
        case 'k':
            state->selected_idx--;
            if (state->selected_idx < 0)
                state->selected_idx = NUM_PARTICLES - 1;
            break;

        case KEY_DOWN:
        case 'j':
            state->selected_idx++;
            if (state->selected_idx >= NUM_PARTICLES)
                state->selected_idx = 0;
            break;

        case KEY_LEFT:
        case 'h':
        case '[':
            if (state->brush_size > 1)
                state->brush_size--;
            break;

        case KEY_RIGHT:
        case 'l':
        case ']':
            if (state->brush_size < 30)
                state->brush_size++;
            break;

        case ' ':
            state->paused = !state->paused;
            break;

        case 'c':
            state->clear_requested = true;
            break;

        case 'q':
        case 27:
            state->quit_requested = true;
            break;
    }
}

void tui_draw(TuiState *state) {
    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 0, 2, " Particles ");

    for (int i = 0; i < NUM_PARTICLES; i++) {
        int pair = color_to_pair(particles[i].color);

        if (i == state->selected_idx) {
            wattron(win, A_REVERSE | A_BOLD);
        }

        wattron(win, COLOR_PAIR(pair));
        mvwprintw(win, i + 1, 2, " %-12s ", particles[i].name);
        wattroff(win, COLOR_PAIR(pair));

        if (i == state->selected_idx) {
            wattroff(win, A_REVERSE | A_BOLD);
        }
    }

    int y = NUM_PARTICLES + 2;
    mvwprintw(win, y, 2, "Brush: %-3d", state->brush_size);
    mvwhline(win, y + 1, 2, 0, 22);

    mvwprintw(win, y + 2, 2, "^v Select");
    mvwprintw(win, y + 3, 2, "<> Brush size");
    mvwprintw(win, y + 4, 2, "Space Pause");
    mvwprintw(win, y + 5, 2, "c Clear  q Quit");

    if (state->paused) {
        wattron(win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(win, y + 2, 16, "PAUSED");
        wattroff(win, COLOR_PAIR(4) | A_BOLD);
    }

    wrefresh(win);
}

ParticleType tui_get_selected_type(TuiState *state) {
    return particles[state->selected_idx].type;
}
