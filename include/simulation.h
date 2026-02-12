#ifndef SIMULATION_H_
#define SIMULATION_H_

#include "particle.h"
#include <stdbool.h>

typedef struct {
    int width;
    int height;
    Particle **grid;
    Particle *pool;
    int *free_list;
    int free_count;
    unsigned int rng_state;
} Simulation;

static unsigned int rng_xorshift(Simulation *sim);
bool sim_init(Simulation *sim);
void sim_cleanup(Simulation *sim);

#endif
