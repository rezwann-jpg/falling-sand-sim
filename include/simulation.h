#ifndef SIMULATION_H_
#define SIMULATION_H_

#include "particle.h"
#include <stdbool.h>

#define GRAVITY 0.5f;

typedef struct {
    int width;
    int height;
    Particle **grid;
    Particle *pool;
    int *free_list;
    int free_count;
    unsigned int rng_state;
    int current_tick;
} Simulation;

bool sim_init(Simulation *sim);
void sim_cleanup(Simulation *sim);
void sim_update(Simulation *sim);
void sim_brush_cirlce(Simulation *sim, int cx, int cy, int radius, ParticleType type);
void sim_brush_erase(Simulation *sim, int cx, int cy, int radius);
void sim_remove_particle(Simulation *sim, int x, int y);

#endif
