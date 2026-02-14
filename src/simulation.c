#include "simulation.h"
#include "common.h"
#include "particle.h"
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

static inline int get_grid_idx(int x, int y) {
    return y * SIM_WIDTH + x;
}

static inline bool in_bounds(int x, int y) {
    return x >= 0 && x < SIM_WIDTH && y >= 0 && y < SIM_HEIGHT;
}

static inline Particle* get_particle(Simulation *sim, int x, int y) {
    if (!in_bounds(x, y))
        return NULL;
    return sim->grid[get_grid_idx(x, y)];
}

static inline void set_particle(Simulation *sim, int x, int y, Particle *p) {
    if (!in_bounds(x, y))
        return;
    sim->grid[get_grid_idx(x, y)] = p;
}

static void swap_particles(Simulation *sim, int x1, int y1, int x2, int y2) {
    int idx1 = get_grid_idx(x1, y1);
    int idx2 = get_grid_idx(x2, y2);

    Particle *temp = sim->grid[idx1];
    sim->grid[idx1] = sim->grid[idx2];
    sim->grid[idx2] = temp;
}

bool sim_spawn_particles(Simulation *sim, int x, int y, ParticleType type) {
    if (!in_bounds(x, y) || get_particle(sim, x, y) || sim->free_count == 0) {
        return false;
    }

    int idx = sim->free_list[--sim->free_count];
    Particle *p = &sim->pool[idx];

    *p = particle_create(type);

    set_particle(sim, x, y, p);
    return true;
}

void sim_remove_particle(Simulation *sim, int x, int y) {
    Particle *p = get_particle(sim, x, y);
    if (!p)
        return;

    int idx = p - sim->pool;
    sim->free_list[sim->free_count++] = idx;
    set_particle(sim, x, y, NULL);
}

static bool can_displace(Particle *a, Particle* b) {
    if (!a)
        return false;
    if (!b)
        return true;

    const ParticleProperties *props_a = particles_get_properties(a->type);
    const ParticleProperties *props_b = particles_get_properties(b->type);

    if (props_b->state == STATE_SOLID)
        return false;

    return props_a->density > props_b->density;
}

static void update_powder(Simulation *sim, int x, int y) {
    Particle *p = get_particle(sim, x, y);

    if (!p)
        return;

    p->vy += GRAVITY;

    if (p->vy > 8.0f)
        p->vy = 8.0f;
    if (p->vx > 4.0f)
        p->vx = 4.0f;
    if (p->vx < -4.0f)
        p->vx = -4.0f;

    int move_y = (int)p->vy;
    if (move_y < 1) move_y = 1;

    for (int i = move_y; i >= 1; i--) {
        if (in_bounds(x, y + i)) {
            Particle *below = get_particle(sim, x, y + i);
            if (can_displace(p, below)) {
                swap_particles(sim, x, y, x, y + i);
                return;
            }
        }
    }

    int dir = (rng_xorshift(sim) % 2) ? -1 : 1;

    if (in_bounds(x + dir, y + 1)) {
        Particle *diag = get_particle(sim, x + dir, y + 1);
        if (can_displace(p, diag)) {
            swap_particles(sim, x, y, x + dir, y + 1);
            p->vx = (float)dir * 0.5f;
            return;
        }
    }

    if (in_bounds(x - dir, y + 1)) {
        Particle *diag = get_particle(sim, x - dir, y + 1);
        if (can_displace(p, diag)) {
            swap_particles(sim, x, y, x - dir, y + 1);
            p->vx = (float)(-dir) * 0.5f;
            return;
        }
    }

    p->vy *= 0.5f;
    p->vx *= 0.8f;
}

static void update_liquid(Simulation *sim, int x, int y) {
    Particle *p = get_particle(sim, x, y);
    if (!p) return;

    const ParticleProperties *props = particles_get_properties(p->type);

    p->vy += GRAVITY;

    if (p->vy > 6.0f) p->vy = 6.0f;
    if (p->vx > 3.0f) p->vx = 3.0f;
    if (p->vx < -3.0f) p->vx = -3.0f;

    int move_y = (int)p->vy;
    if (move_y < 1) move_y = 1;

    for (int i = move_y; i >= 1; i--) {
        if (in_bounds(x, y + i)) {
            Particle *below = get_particle(sim, x, y + i);
            if (can_displace(p, below)) {
                swap_particles(sim, x, y, x, y + i);
                return;
            }
        }
    }

    int dir = (rng_xorshift(sim) % 2) ? -1 : 1;

    Particle *diag1 = get_particle(sim, x + dir, y + 1);
    if (can_displace(p, diag1)) {
        swap_particles(sim, x, y, x + dir, y + 1);
        return;
    }

    Particle *diag2 = get_particle(sim, x - dir, y + 1);
    if (can_displace(p, diag2)) {
        swap_particles(sim, x, y, x - dir, y + 1);
        return;
    }

    int flow_distance = (int)(3.0f * (1.0f - props->viscosity)) + 1;
    int flow_dir = (rng_xorshift(sim) % 2) ? -1 : 1;

    for (int d = 0; d < 2; d++) {
        int current_dir = (d == 0) ? flow_dir : -flow_dir;

        for (int i = 1; i <= flow_distance; i++) {
            int nx = x + i * current_dir;
            Particle *side = get_particle(sim, nx, y);

            if (!in_bounds(nx, y)) break;

            if (can_displace(p, side)) {
                swap_particles(sim, x, y, nx, y);
                p->vx = (float)current_dir;
                return;
            } else if (side) {
                break;
            }
        }
    }

    p->vy *= 0.3f;
    p->vx *= 0.9f;
}

// static void update_sand(Simulation *sim, int x, int y) {
//     if (in_bounds(x, y + 1) && !get_particle(sim, x, y + 1)) {
//         swap_particles(sim, x, y, x, y + 1);
//         return;
//     }

//     int dir = (rng_xorshift(sim) % 2) ? -1 : 1;

//     if (in_bounds(x + dir, y + 1) && !get_particle(sim, x + dir, y + 1)) {
//         swap_particles(sim, x, y, x + dir, y + 1);
//         return;
//     }

//     if (in_bounds(x - dir, y + 1) && !get_particle(sim, x - dir, y + 1)) {
//         swap_particles(sim, x, y, x - dir, y + 1);
//     }
// }

// static void update_water(Simulation *sim, int x, int y) {
//     if (in_bounds(x, y + 1) && !get_particle(sim, x, y + 1)) {
//         swap_particles(sim, x, y, x, y + 1);
//         return;
//     }

//     int dir = (rng_xorshift(sim) % 2) ? -1 : 1;

//     if (in_bounds(x + dir, y + 1) && !get_particle(sim, x + dir, y + 1)) {
//         swap_particles(sim, x, y, x + dir, y + 1);
//         return;
//     }

//     if (in_bounds(x - dir, y + 1) && !get_particle(sim, x - dir, y + 1)) {
//         swap_particles(sim, x, y, x - dir, y + 1);
//         return;
//     }

//     int flow_dir = (rng_xorshift(sim) % 2) ? -1 : 1;

//     if (in_bounds(x + flow_dir, y) && !get_particle(sim, x + flow_dir, y)) {
//         swap_particles(sim, x, y, x + flow_dir, y);
//         return;
//     }

//     if (in_bounds(x - flow_dir, y) && !get_particle(sim, x - flow_dir, y)) {
//         swap_particles(sim, x, y, x - flow_dir, y);
//     }
// }

void update_particle(Simulation *sim, int x, int y) {
    Particle *p = get_particle(sim, x, y);
    if (!p || p->updated)
        return;

    p->updated = true;

    const ParticleProperties *props = particles_get_properties(p->type);

    switch (props->state) {
        case STATE_POWDER:
            update_powder(sim, x, y);
            break;
        case STATE_LIQUID:
            update_liquid(sim, x, y);
            break;
    }
}

void sim_update(Simulation *sim) {
    // for (int y = SIM_HEIGHT - 1; y >= 0; y--) {
    //     if (y % 2 == 0) {
    //         for (int x = 0; x < SIM_WIDTH; x++) {
    //             Particle *p = get_particle(sim, x, y);
    //             if (!p)
    //                 continue;

    //             switch (p->type) {
    //                 case PARTICLE_SAND:
    //                     update_sand(sim, x, y);
    //                     break;
    //                 case PARTICLE_WATER:
    //                     update_water(sim, x, y);
    //                     break;
    //                 default:
    //                     break;
    //             }
    //         }
    //     }
    //     else {
    //         for (int x = SIM_WIDTH - 1; x >= 0; x--) {
    //             Particle *p = get_particle(sim, x, y);
    //             if (!p)
    //                 continue;

    //             switch (p->type) {
    //                 case PARTICLE_SAND:
    //                     update_sand(sim, x, y);
    //                     break;
    //                 case PARTICLE_WATER:
    //                     update_water(sim, x, y);
    //                     break;
    //                 default:
    //                     break;
    //             }
    //         }
    //     }
    // }

    sim->current_tick++;

    for (int i = 0; i < SIM_WIDTH * SIM_HEIGHT; i++) {
        if (sim->grid[i]) {
            sim->grid[i]->updated = false;
        }
    }

    bool left_to_right = (sim->current_tick % 2) == 0;

    for (int y = SIM_HEIGHT - 1; y >= 0; y--) {
        if (left_to_right) {
            for (int x = 0; x < SIM_WIDTH; x++) {
                update_particle(sim, x, y);
            }
        } else {
            for (int x = SIM_WIDTH - 1; x >= 0; x--) {
                update_particle(sim, x, y);
            }
        }
    }
}

void sim_brush_cirlce(Simulation *sim, int cx, int cy, int radius, ParticleType type) {
    int r2 = radius * radius;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > r2)
                continue;

            int x = cx + dx;
            int y = cy + dy;

            sim_spawn_particles(sim, x, y, type);
        }
    }
}

void sim_brush_erase(Simulation *sim, int cx, int cy, int radius) {
    int r2 = radius * radius;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > r2)
                continue;

            int x = cx + dx;
            int y = cy + dy;

            sim_remove_particle(sim, x, y);
        }
    }
}
