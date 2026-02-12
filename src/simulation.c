#include "simulation.h"
#include "common.h"
#include <stdlib.h>
#include <time.h>

static unsigned int rng_xorshift(Simulation *sim) {
    sim->rng_state ^= sim->rng_state << 13;
    sim->rng_state ^= sim->rng_state >> 17;
    sim->rng_state ^= sim->rng_state << 5;
    return sim->rng_state;
}

bool sim_init(Simulation *sim) {
    sim->width = SIM_WIDTH;
    sim->height = SIM_HEIGHT;
    sim->rng_state = (unsigned int)time(NULL);

    sim->grid = (Particle **)calloc(SIM_WIDTH * SIM_HEIGHT, sizeof(Particle *));
    if (!sim->grid)
        return false;

    int max_particles = SIM_WIDTH * SIM_HEIGHT;
    sim->pool = (Particle *)malloc(max_particles * sizeof(Particle));
    sim->free_list = (int *)malloc(max_particles * sizeof(int));

    if (!sim->pool || !sim->free_list) {
        sim_cleanup(sim);
        return false;
    }

    sim->free_count = max_particles;
    for (int i = 0; i < max_particles; i++) {
        sim->free_list[i] = i;
    }

    return true;
}

void sim_cleanup(Simulation *sim) {
    free(sim->grid);
    free(sim->pool);
    free(sim->free_list);

    sim->grid = NULL;
    sim->pool = NULL;
    sim->free_list = NULL;
}
