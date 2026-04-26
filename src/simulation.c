#include "simulation.h"
#include "common.h"
#include "color.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

static unsigned int rng_xorshift(Simulation *sim) {
    sim->rng_state ^= sim->rng_state << 13;
    sim->rng_state ^= sim->rng_state >> 17;
    sim->rng_state ^= sim->rng_state << 5;
    return sim->rng_state;
}

static float rng_float(Simulation *sim) {
    return (float)rng_xorshift(sim) / (float)UINT32_MAX;
}

bool sim_init(Simulation *sim) {
    sim->width = SIM_WIDTH;
    sim->height = SIM_HEIGHT;
    sim->rng_state = (unsigned int)time(NULL);
    sim->current_tick = 0;

    particles_init_colors();

    sim->grid_current = calloc(SIM_WIDTH * SIM_HEIGHT, sizeof(Particle));
    sim->grid_next = calloc(SIM_WIDTH * SIM_HEIGHT, sizeof(Particle));
    if (!sim->grid_current || !sim->grid_next) {
        return false;
    }

    return true;
}

void sim_cleanup(Simulation *sim) {
    free(sim->grid_current);
    free(sim->grid_next);
    sim->grid_current = NULL;
    sim->grid_next = NULL;
}

static inline int idx(int x, int y) {
    return y * SIM_WIDTH + x;
}

static inline bool in_bounds(int x, int y) {
    return x >= 0 && x < SIM_WIDTH && y >= 0 && y < SIM_HEIGHT;
}

static bool is_empty_current(Simulation *sim, int x, int y) {
    return in_bounds(x, y) && !sim->grid_current[idx(x, y)].active;
}

static void swap_particles(Simulation *sim, int x1, int y1, int x2, int y2) {
    int idx1 = idx(x1, y1);
    int idx2 = idx(x2, y2);

    Particle temp = sim->grid_next[idx1];
    sim->grid_next[idx1] = sim->grid_next[idx2];
    sim->grid_next[idx2] = temp;

    sim->grid_current[idx1].handled = true;
    sim->grid_current[idx2].handled = true;
    
    if (sim->grid_next[idx1].type != PARTICLE_NONE)
        sim->grid_next[idx1].handled = true;
    if (sim->grid_next[idx2].type != PARTICLE_NONE)
        sim->grid_next[idx2].handled = true;
}

bool sim_spawn_particles(Simulation *sim, int x, int y, ParticleType type) {
    if (!in_bounds(x, y)) return false;

    Particle *p = &sim->grid_current[idx(x,y)];
    if (p->active) return false;

    *p = particle_create(type, &sim->rng_state);
    return true;
}

void sim_remove_particle(Simulation *sim, int x, int y) {
    if (!in_bounds(x,y)) return;
    sim->grid_current[idx(x,y)].active = false;
    sim->grid_current[idx(x, y)].type = PARTICLE_NONE;
    sim->grid_current[idx(x, y)].vx = 0;
    sim->grid_current[idx(x, y)].vy = 0;
}

static void transform_particle(Simulation *sim, int x, int y, ParticleType new_type) {
    Particle *p_next = &sim->grid_next[idx(x, y)];
    
    if (new_type == PARTICLE_NONE) {
        p_next->active = false;
        p_next->type = PARTICLE_NONE;
        p_next->handled = true;
        sim->grid_current[idx(x, y)].handled = true;
        return;
    }

    float old_temp = p_next->temperature;
    *p_next = particle_create(new_type, &sim->rng_state);
    p_next->temperature = old_temp;
    p_next->handled = true;
    sim->grid_current[idx(x, y)].handled = true;
}

static void update_temperature(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];
    Particle *next = &sim->grid_next[idx(x, y)];

    const ParticleProperties *props = particles_get_properties(curr->type);

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    float total_transfer = 0.0f;
    int neighbor_count = 0;

    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        
        if (!in_bounds(nx, ny)) continue;
        
        Particle *neighbor = &sim->grid_current[idx(nx, ny)];
        float neighbor_temp = neighbor->active ? neighbor->temperature : AMBIENT_TEMP;
        float diff = neighbor_temp - curr->temperature;

        float conductivity = props->thermal_conductivity;
        if (neighbor->active) {
            const ParticleProperties *n_props = particles_get_properties(neighbor->type);
            conductivity = (conductivity + n_props->thermal_conductivity) * 0.5f;
        }

        total_transfer += diff * conductivity * TEMP_TRANSFER_RATE;
        neighbor_count++;
    }

    if (neighbor_count > 0) {
        next->temperature += total_transfer / neighbor_count;
    }

    next->temperature += (AMBIENT_TEMP - next->temperature) * 0.001f;
}
static void grow_plant_session(Simulation *sim, int root_x, int root_y, int energy) {
    int cx = root_x;
    int cy = root_y;
    
    while(energy > 0) {
        int dx = 0;
        int dy = -1;
        float r = rng_float(sim);
        if (r < 0.1f) dx = -1;
        else if (r < 0.2f) dx = 1;
        
        cx += dx;
        cy += dy;
        
        if (!in_bounds(cx, cy)) break;
        
        ParticleType pt = sim->grid_current[idx(cx, cy)].type;
        
        if (pt == PARTICLE_NONE) {
            bool flower = (energy == 1 || rng_float(sim) < 0.05f);
            if (flower) {
                for(int fd=-1; fd<=1; fd++) {
                    for(int fx=-1; fx<=1; fx++) {
                        if (fx*fx + fd*fd > 1) continue;
                        int fnx = cx + fx;
                        int fny = cy + fd;
                        if (in_bounds(fnx, fny) && is_empty_current(sim, fnx, fny) && !sim->grid_current[idx(fnx, fny)].handled) {
                            sim->grid_current[idx(fnx, fny)] = particle_create(PARTICLE_FLOWER, &sim->rng_state);
                            sim->grid_next[idx(fnx, fny)] = sim->grid_current[idx(fnx, fny)];
                        }
                    }
                }
                break;
            } else {
                sim->grid_current[idx(cx, cy)] = particle_create(PARTICLE_PLANT, &sim->rng_state);
                sim->grid_next[idx(cx, cy)] = sim->grid_current[idx(cx, cy)];
                energy--;
            }
        } else if (pt == PARTICLE_PLANT || pt == PARTICLE_FLOWER) {
            continue;
        } else {
            if (is_empty_current(sim, cx - 1, cy)) cx--;
            else if (is_empty_current(sim, cx + 1, cy)) cx++;
            else break;
        }
    }
}

static void check_reactions(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];
    Particle *next = &sim->grid_next[idx(x, y)];

    if (curr->type == PARTICLE_WOOD || curr->type == PARTICLE_PLANT || curr->type == PARTICLE_FLOWER || curr->type == PARTICLE_ASH || curr->type == PARTICLE_DEBRIS) {
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int i=0; i<4; i++) {
            int nx = x+dx[i]; int ny = y+dy[i];
            if (in_bounds(nx,ny) && sim->grid_current[idx(nx,ny)].active && sim->grid_current[idx(nx,ny)].type == PARTICLE_WATER) {
                if (rng_float(sim) < 0.005f) {
                    transform_particle(sim, x, y, PARTICLE_SPORE);
                    return;
                }
                break;
            }
        }
    }

    if (curr->type == PARTICLE_SPORE) {
        int water_count = 0;
        int dx[] = {-1, 1, 0, 0, -1, 1, -1, 1};
        int dy[] = {0, 0, -1, 1, -1, -1, 1, 1};
        for (int i=0; i<8; i++) {
            int nx = x+dx[i]; int ny = y+dy[i];
            if (in_bounds(nx,ny) && sim->grid_current[idx(nx,ny)].active && sim->grid_current[idx(nx,ny)].type == PARTICLE_WATER) {
                water_count++;
            }
        }
        if (water_count >= 2) {
            transform_particle(sim, x, y, PARTICLE_SPORELING);
            return;
        }
    }

    const ParticleProperties *props = particles_get_properties(curr->type);

    // Boiling
    if (props->boiling_point > 0 && curr->temperature >= props->boiling_point) {
        if (props->state == STATE_LIQUID) {
            transform_particle(sim, x, y, props->boils_into);
            return;
        }
    }

    // Burning
    if (props->flammability > 0 && props->ignition_point > 0) {
        if (curr->temperature >= props->ignition_point || curr->burning) {
            next->burning = true;
            next->temperature += 10.0f * props->flammability;

            if (rng_float(sim) < props->flammability * 0.1f) {
                if (is_empty_current(sim, x, y - 1) && rng_float(sim) < 0.3f && !sim->grid_current[idx(x, y-1)].handled) {
                    sim->grid_next[idx(x, y - 1)] = particle_create(PARTICLE_SMOKE, &sim->rng_state);
                    sim->grid_current[idx(x, y - 1)].handled = true;
                }
                transform_particle(sim, x, y, props->burns_into);
                return;
            }
        }
    }

    if (curr->type == PARTICLE_FIRE) {
        int dx[] = {-1, 1, 0, 0, -1, 1, -1, 1};
        int dy[] = {0, 0, -1, 1, -1, -1, 1, 1};

        for (int i = 0; i < 8; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (!in_bounds(nx, ny)) continue;

            Particle *neighbor = &sim->grid_current[idx(nx, ny)];
            if (neighbor->active) {
                const ParticleProperties *n_props = particles_get_properties(neighbor->type);
                sim->grid_next[idx(nx, ny)].temperature += 20.0f; // Heat neighbors

                if (n_props->flammability > 0 && rng_float(sim) < n_props->flammability * 0.05f) {
                    sim->grid_next[idx(nx, ny)].burning = true;
                }
            }
        }
    }

    if (curr->type == PARTICLE_WATER) {
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};

        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (!in_bounds(nx, ny)) continue;
            
            Particle *neighbor = &sim->grid_current[idx(nx, ny)];
            if (neighbor->active) {
                if (neighbor->type == PARTICLE_FIRE || neighbor->type == PARTICLE_LAVA) {
                    // Turn neighbor into none or stone, turn self to steam
                    if (neighbor->type == PARTICLE_FIRE) {
                        sim->grid_next[idx(nx, ny)].active = false;
                        sim->grid_next[idx(nx, ny)].type = PARTICLE_NONE;
                    } else if (neighbor->type == PARTICLE_LAVA) {
                        sim->grid_next[idx(nx, ny)] = particle_create(PARTICLE_STONE, &sim->rng_state);
                    }
                    sim->grid_current[idx(nx, ny)].handled = true;
                    transform_particle(sim, x, y, PARTICLE_STEAM);
                    return;
                } else if (neighbor->type == PARTICLE_SAND) {
                    // Water + Sand -> Wet Sand + Empty (water absorbed)
                    if (dy[i] == 1 || rng_float(sim) < 0.05f) {
                        sim->grid_next[idx(nx, ny)] = particle_create(PARTICLE_WET_SAND, &sim->rng_state);
                        sim->grid_current[idx(nx, ny)].handled = true;
                        transform_particle(sim, x, y, PARTICLE_NONE);
                        return;
                    }
                } else if (neighbor->type == PARTICLE_PLANT) {
                    transform_particle(sim, x, y, PARTICLE_NONE);
                    grow_plant_session(sim, nx, ny, 3);
                    return;
                }
                sim->grid_next[idx(nx, ny)].burning = false;
            }
        }
    }

    if (curr->type == PARTICLE_WET_SAND) {
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (!in_bounds(nx, ny)) continue;
            Particle *neighbor = &sim->grid_current[idx(nx, ny)];
            if (neighbor->active && neighbor->type == PARTICLE_PLANT) {
                if (rng_float(sim) < 0.05f) {
                    transform_particle(sim, x, y, PARTICLE_SAND);
                    grow_plant_session(sim, nx, ny, 2);
                    return;
                }
            }
        }
    }

    if (curr->type == PARTICLE_SEED) {
        bool has_water = false;
        bool has_soil = false;
        bool touches_plant = false;
        int dx[] = {-1, 1, 0, 0, -1, 1, -1, 1};
        int dy[] = {0, 0, -1, 1, -1, -1, 1, 1};
        for(int i=0; i<8; i++) {
            int nx = x+dx[i];
            int ny = y+dy[i];
            if (in_bounds(nx, ny)) {
                ParticleType pt = sim->grid_current[idx(nx, ny)].type;
                if (pt == PARTICLE_WATER) has_water = true;
                if (pt == PARTICLE_SAND) has_soil = true;
                if (pt == PARTICLE_WET_SAND) {
                    has_water = true;
                    has_soil = true;
                }
                if (pt == PARTICLE_PLANT || pt == PARTICLE_FLOWER) touches_plant = true;
            }
        }
        if (has_water && has_soil && rng_float(sim) < 0.05f) {
            transform_particle(sim, x, y, PARTICLE_PLANT);
            grow_plant_session(sim, x, y, 5);
            return;
        } else if (touches_plant) {
            transform_particle(sim, x, y, PARTICLE_PLANT);
            return;
        }
    }
}

static void update_lifetime(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];
    Particle *next = &sim->grid_next[idx(x, y)];
    
    if (curr->type == PARTICLE_SPORELING) return;
    
    if (curr->lifetime < 0) return;

    next->lifetime--;

    if (next->lifetime <= 0) {
        if (curr->type == PARTICLE_FIRE) {
            if (rng_float(sim) < 0.5f) {
                transform_particle(sim, x, y, PARTICLE_SMOKE);
                return;
            }
        }
        transform_particle(sim, x, y, PARTICLE_NONE);
        return;
    }

    if (curr->type == PARTICLE_FIRE) {
        const ParticleProperties *props = particles_get_properties(PARTICLE_FIRE);
        float life_ratio = (float)next->lifetime / (float)props->lifetime;
        next->color = color_lerp(props->color_min, props->color_max, life_ratio);
        next->color.a = (unsigned int)(150 + 105 * life_ratio);
        next->render_color = (next->color.a << 24) |
                             (next->color.b << 16) |
                             (next->color.g << 8)  |
                             (next->color.r);
    }
}

static bool can_displace(Simulation *sim, int nx, int ny, Particle *a, Particle* b_curr) {
    if (sim->grid_next[idx(nx, ny)].type == PARTICLE_NONE) {
        if (sim->grid_next[idx(nx, ny)].handled) return false;
        return true;
    }
    
    if (sim->grid_current[idx(nx, ny)].handled) return false;
    
    if (!a->active) return false;
    if (!b_curr->active) return true;

    const ParticleProperties *props_a = particles_get_properties(a->type);
    const ParticleProperties *props_b = particles_get_properties(b_curr->type);

    if (props_b->state == STATE_SOLID) return false;

    return props_a->density > props_b->density;
}

static void update_powder(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];
    Particle *next = &sim->grid_next[idx(x, y)];

    const ParticleProperties *props = particles_get_properties(curr->type);

    next->vy += GRAVITY;
    next->vx *= (1.0f - props->friction * 0.1f);
    next->vy *= 0.99f;

    if (next->vy > 8.0f) next->vy = 8.0f;
    if (next->vx > 4.0f) next->vx = 4.0f;
    if (next->vx < -4.0f) next->vx = -4.0f;

    int move_y = (int)next->vy;
    if (move_y < 1) move_y = 1;

    int final_i = 0;
    for (int i = 1; i <= move_y; i++) {
        if (in_bounds(x, y + i)) {
            Particle *below = &sim->grid_current[idx(x, y + i)];
            if (can_displace(sim, x, y + i, next, below)) {
                final_i = i;
            } else {
                if (below->active) {
                    next->vy *= -props->bounciness;
                }
                break;
            }
        } else {
            break;
        }
    }
    if (final_i > 0) {
        swap_particles(sim, x, y, x, y + final_i);
        return;
    }

    int dir = (rng_xorshift(sim) % 2) ? -1 : 1;
    if (in_bounds(x + dir, y + 1)) {
        Particle *diag = &sim->grid_current[idx(x + dir, y + 1)];
        if (can_displace(sim, x + dir, y + 1, next, diag)) {
            swap_particles(sim, x, y, x + dir, y + 1);
            next->vx = (float)dir * 0.5f;
            return;
        }
    }

    if (in_bounds(x - dir, y + 1)) {
        Particle *diag = &sim->grid_current[idx(x - dir, y + 1)];
        if (can_displace(sim, x - dir, y + 1, next, diag)) {
            swap_particles(sim, x, y, x - dir, y + 1);
            next->vx = (float)(-dir) * 0.5f;
            return;
        }
    }

    next->vy = 0;
}

static void update_liquid(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];
    Particle *next = &sim->grid_next[idx(x, y)];

    const ParticleProperties *props = particles_get_properties(curr->type);

    next->vy += GRAVITY;
    float viscosity_factor = 1.0f - props->viscosity * 0.3f;
    next->vx *= viscosity_factor;
    next->vy *= viscosity_factor;

    if (next->vy > 6.0f) next->vy = 6.0f;
    if (next->vx > 3.0f) next->vx = 3.0f;
    if (next->vx < -3.0f) next->vx = -3.0f;

    int move_y = (int)next->vy;
    if (move_y < 1) move_y = 1;

    int final_i = 0;
    for (int i = 1; i <= move_y; i++) {
        if (in_bounds(x, y + i)) {
            Particle *below = &sim->grid_current[idx(x, y + i)];
            if (can_displace(sim, x, y + i, next, below)) {
                final_i = i;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    if (final_i > 0) {
        swap_particles(sim, x, y, x, y + final_i);
        return;
    }

    int dir = (rng_xorshift(sim) % 2) ? -1 : 1;
    if (in_bounds(x + dir, y + 1)) {
        Particle *diag1 = &sim->grid_current[idx(x + dir, y + 1)];
        if (can_displace(sim, x + dir, y + 1, next, diag1)) {
            swap_particles(sim, x, y, x + dir, y + 1);
            return;
        }
    }

    if (in_bounds(x - dir, y + 1)) {
        Particle *diag2 = &sim->grid_current[idx(x - dir, y + 1)];
        if (can_displace(sim, x - dir, y + 1, next, diag2)) {
            swap_particles(sim, x, y, x - dir, y + 1);
            return;
        }
    }

    int flow_distance = (int)(3.0f * (1.0f - props->viscosity)) + 1;
    int flow_dir = (rng_xorshift(sim) % 2) ? -1 : 1;

    for (int d = 0; d < 2; d++) {
        int current_dir = (d == 0) ? flow_dir : -flow_dir;

        int final_nx = x;
        for (int i = 1; i <= flow_distance; i++) {
            int nx = x + i * current_dir;
            if (!in_bounds(nx, y)) break;

            Particle *side = &sim->grid_current[idx(nx, y)];
            if (can_displace(sim, nx, y, next, side)) {
                final_nx = nx;
            } else if (side->active) {
                break;
            }
        }
        
        if (final_nx != x) {
            float new_vx = (float)current_dir;
            swap_particles(sim, x, y, final_nx, y);
            sim->grid_next[idx(final_nx, y)].vx = new_vx;
            return;
        }
    }

    next->vy = 0;
}

static void update_gas(Simulation *sim, int x, int y) {
    Particle *next = &sim->grid_next[idx(x, y)];

    next->vy -= GRAVITY * 0.3f;
    next->vx += (rng_float(sim) - 0.5f) * 0.5f;
    next->vy += (rng_float(sim) - 0.5f) * 0.3f;

    next->vx *= 0.9f;
    next->vy *= 0.9f;

    if (next->vy < -3.0f) next->vy = -3.0f;
    if (next->vy > 2.0f) next->vy = 2.0f;
    if (next->vx > 2.0f) next->vx = 2.0f;
    if (next->vx < -2.0f) next->vx = -2.0f;

    int move_y = (int)next->vy;
    if (move_y < 0) {
        for (int i = -1; i >= move_y; i--) {
            if (in_bounds(x, y + i)) {
                Particle *above = &sim->grid_current[idx(x, y + i)];
                if (can_displace(sim, x, y + i, next, above)) {
                    swap_particles(sim, x, y, x, y + i);
                    return;
                }
            }
        }
    }

    int dirs[4][2] = {{-1, -1}, {1, -1}, {-1, 0}, {1, 0}};
    int start = rng_xorshift(sim) % 4;

    for (int i = 0; i < 4; i++) {
        int d_idx = (start + i) % 4;
        int nx = x + dirs[d_idx][0];
        int ny = y + dirs[d_idx][1];

        if (in_bounds(nx, ny)) {
            Particle *d = &sim->grid_current[idx(nx, ny)];
            if (can_displace(sim, nx, ny, next, d)) {
                swap_particles(sim, x, y, nx, ny);
                return;
            }
        }
    }

    if (in_bounds(x, y - 1)) {
        Particle *up = &sim->grid_current[idx(x, y - 1)];
        if (can_displace(sim, x, y - 1, next, up)) {
            swap_particles(sim, x, y, x, y - 1);
        }
    }
}

static void update_dpl(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];
    Particle *next = &sim->grid_next[idx(x, y)];
    
    // Tuned interaction matrix: A chases B, B chases C, C chases A
    // Positive = attract, Negative = repel.
    // [i][j] = how type i responds to type j.
    static const float matrix[3][3] = {
        { 0.5f,  1.2f, -1.0f}, // A likes A, chases B, runs from C
        {-1.0f,  0.5f,  1.2f}, // B runs from A, likes B, chases C
        { 1.2f, -1.0f,  0.5f}  // C chases A, runs from B, likes C
    };
    
    int my_idx = curr->type - PARTICLE_DPL_A;
    
    float fx = 0, fy = 0;
    int radius = 5; // Larger radius for more complex interactions
    int neighbors = 0;
    
    // 1. Gather Forces
    for(int dy = -radius; dy <= radius; dy++) {
        for(int dx = -radius; dx <= radius; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            if (in_bounds(nx, ny) && sim->grid_current[idx(nx, ny)].active) {
                ParticleType nt = sim->grid_current[idx(nx, ny)].type;
                if (nt >= PARTICLE_DPL_A && nt <= PARTICLE_DPL_C) {
                    int their_idx = nt - PARTICLE_DPL_A;
                    float force_mag = matrix[my_idx][their_idx];
                    
                    float dist2 = (float)(dx*dx + dy*dy);
                    if (dist2 == 0) continue;
                    float dist = sqrtf(dist2);
                    
                    // Very short range strong repulsion to prevent overlapping clumps
                    if (dist < 2.0f) {
                        force_mag -= 3.0f; // Push away strongly
                    }
                    
                    fx += (force_mag * dx / dist);
                    fy += (force_mag * dy / dist);
                    neighbors++;
                }
            }
        }
    }
    
    // 2. Physics: Apply Momentum and Friction
    // Add tiny jitter to prevent perfect locks
    fx += (rng_float(sim) - 0.5f) * 0.2f;
    fy += (rng_float(sim) - 0.5f) * 0.2f;

    next->vx += fx * 0.2f;
    next->vy += fy * 0.2f;
    
    // Friction (0.5 means it loses half speed each tick, causing smooth glide/stop)
    next->vx *= 0.5f;
    next->vy *= 0.5f;
    
    // 3. Movement
    // Because this is a discrete grid, float velocities must map to integer steps
    int dx_int = (int)next->vx;
    int dy_int = (int)next->vy;
    
    // Fractional remainder translates to probability of taking one extra step
    float rem_x = next->vx - dx_int;
    float rem_y = next->vy - dy_int;
    
    if (rng_float(sim) < fabsf(rem_x)) dx_int += (rem_x > 0 ? 1 : -1);
    if (rng_float(sim) < fabsf(rem_y)) dy_int += (rem_y > 0 ? 1 : -1);

    // Clamp speed to prevent skipping too far (max 1 for smoothness in CA)
    if (dx_int >  1) dx_int =  1;
    if (dx_int < -1) dx_int = -1;
    if (dy_int >  1) dy_int =  1;
    if (dy_int < -1) dy_int = -1;
    
    // Liveliness criteria update: 
    // They survive best in small interconnected clusters
    if (neighbors < 1 || neighbors > 25) {
        next->lifetime -= 3;
    } else {
        next->lifetime += 5;
        if (next->lifetime > 1000) next->lifetime = 1000;
    }
    
    // Evaporate rather than turn to ash, so it doesn't leave "ghost" blockers
    if (next->lifetime <= 0) {
        transform_particle(sim, x, y, PARTICLE_NONE);
        return;
    }
    
    // Final check to move
    if (dx_int != 0 || dy_int != 0) {
        int nx = x + dx_int;
        int ny = y + dy_int;
        if (in_bounds(nx, ny)) {
            Particle *d = &sim->grid_current[idx(nx, ny)];
            if (can_displace(sim, nx, ny, next, d)) {
                swap_particles(sim, x, y, nx, ny);
            } else if (dx_int != 0 && can_displace(sim, nx, y, next, &sim->grid_current[idx(nx, y)])) {
                swap_particles(sim, x, y, nx, y); // try horiz
            } else if (dy_int != 0 && can_displace(sim, x, ny, next, &sim->grid_current[idx(x, ny)])) {
                swap_particles(sim, x, y, x, ny); // try vert
            }
            // else blocked, momentum dissipates a bit more
            if (sim->grid_current[idx(x, y)].handled) { // Wait, next pointer becomes invalid if we swap!
                // if swapped, velocity transferred via swap_particles but we act on `next`
            } else {
                 next->vx *= 0.5f; 
                 next->vy *= 0.5f;
            }
        } else {
            // Screen edge bounce
            next->vx *= -0.8f;
            next->vy *= -0.8f;
        }
    }
}

static void update_solid(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];
    Particle *next = &sim->grid_next[idx(x, y)];

    if (curr->type == PARTICLE_PLANT || curr->type == PARTICLE_FLOWER) {
        if (rng_float(sim) < 0.001f) { // Seed drop
            int nx = x;
            int ny = y - 1; // Drop seed from upwards, or drop seed left/right
            if (rng_float(sim) > 0.5f) {
                nx = x + ((rng_xorshift(sim)%2)?1:-1);
                ny = y;
            }
            if (in_bounds(nx, ny) && is_empty_current(sim, nx, ny) && !sim->grid_current[idx(nx, ny)].handled) {
                sim->grid_next[idx(nx, ny)] = particle_create(PARTICLE_SEED, &sim->rng_state);
                sim->grid_current[idx(nx, ny)].handled = true;
            }
        }
    } else if (curr->type == PARTICLE_SPORELING) {
        bool in_water = false;
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int i=0; i<4; i++) {
            int nx = x+dx[i]; int ny=y+dy[i];
            if (in_bounds(nx, ny) && sim->grid_current[idx(nx,ny)].active && sim->grid_current[idx(nx,ny)].type == PARTICLE_WATER) {
                in_water = true; break;
            }
        }
        
        next->lifetime -= (in_water ? 5 : 1);
        if (next->lifetime <= 0) {
            transform_particle(sim, x, y, PARTICLE_DEBRIS);
            return;
        }

        int dirs[8][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {1,-1}, {-1,1}, {1,1}};
        for (int i=0; i<8; i++) {
            int nx = x+dirs[i][0]; int ny=y+dirs[i][1];
            if (in_bounds(nx, ny) && sim->grid_current[idx(nx,ny)].active && !sim->grid_current[idx(nx,ny)].handled) {
                ParticleType nt = sim->grid_current[idx(nx,ny)].type;
                if (nt == PARTICLE_WOOD || nt == PARTICLE_PLANT || nt == PARTICLE_FLOWER || nt == PARTICLE_ASH || nt == PARTICLE_DEBRIS) {
                    sim->grid_next[idx(nx,ny)].active = false;
                    sim->grid_next[idx(nx,ny)].type = PARTICLE_NONE;
                    sim->grid_current[idx(nx,ny)].handled = true;
                    next->lifetime += 50;
                    break;
                }
            }
        }

        if (next->lifetime >= 180) {
            for (int i=0; i<4; i++) {
                int nx = x+dx[i]; int ny=y+dy[i];
                if (in_bounds(nx, ny) && is_empty_current(sim, nx, ny) && !sim->grid_current[idx(nx,ny)].handled) {
                    next->lifetime /= 2;
                    sim->grid_next[idx(nx,ny)] = particle_create(PARTICLE_SPORELING, &sim->rng_state);
                    sim->grid_next[idx(nx,ny)].lifetime = next->lifetime;
                    sim->grid_next[idx(nx,ny)].vx = (float)((rng_xorshift(sim) % 2) ? 1 : -1);
                    sim->grid_current[idx(nx,ny)].handled = true;
                    break;
                }
            }
        }

        if (sim->current_tick % 2 == 0) {
            Color c2 = COLOR_SPORELING_2;
            next->render_color = (c2.a<<24) | (c2.b<<16) | (c2.g<<8) | c2.r;
        } else {
            next->render_color = particles_get_packed_color(PARTICLE_SPORELING);
        }

        if (in_bounds(x, y+1) && sim->grid_current[idx(x, y+1)].active) {
            int dir = (next->vx > 0) ? 1 : -1;
            if (next->vx == 0) dir = (rng_xorshift(sim) % 2) ? 1 : -1;
            
            int nx = x + dir;
            if (in_bounds(nx, y)) {
                Particle *side = &sim->grid_current[idx(nx, y)];
                if (can_displace(sim, nx, y, next, side)) {
                    swap_particles(sim, x, y, nx, y);
                    sim->grid_next[idx(nx, y)].vx = (float)dir;
                } else {
                    next->vx = (float)(-dir);
                }
            } else {
                next->vx = (float)(-dir);
            }
        } else if (in_bounds(x, y+1)) {
            Particle *below = &sim->grid_current[idx(x, y+1)];
            if (can_displace(sim, x, y+1, next, below)) {
                swap_particles(sim, x, y, x, y+1);
            }
        }
    } else if (curr->type == PARTICLE_ANT) {
        if (in_bounds(x, y+1) && sim->grid_current[idx(x, y+1)].active) {
            int dir = (next->vx > 0) ? 1 : -1;
            if (next->vx == 0) dir = (rng_xorshift(sim) % 2) ? 1 : -1;
            
            int nx = x + dir;
            if (in_bounds(nx, y)) {
                Particle *side = &sim->grid_current[idx(nx, y)];
                if (can_displace(sim, nx, y, next, side)) {
                    swap_particles(sim, x, y, nx, y);
                    sim->grid_next[idx(nx, y)].vx = dir;
                } else {
                    next->vx = -dir; // wall
                }
            } else {
                next->vx = -dir; // boundary
            }
        } else if (in_bounds(x, y+1)) {
            Particle *below = &sim->grid_current[idx(x, y+1)];
            if (can_displace(sim, x, y+1, next, below)) {
                swap_particles(sim, x, y, x, y+1);
            }
        }
    }
}

void update_particle(Simulation *sim, int x, int y) {
    Particle *curr = &sim->grid_current[idx(x, y)];

    if (!curr->active || curr->handled)
        return;

    const ParticleProperties *props = particles_get_properties(curr->type);

    update_temperature(sim, x, y);
    
    // Refresh ptr incase it handled itself incorrectly, but temperature doesnt swap.
    if (sim->grid_current[idx(x, y)].handled) return;
    
    check_reactions(sim, x, y);

    if (sim->grid_current[idx(x, y)].handled) return;

    update_lifetime(sim, x, y);

    if (sim->grid_current[idx(x, y)].handled) return;

    switch (props->state) {
        case STATE_POWDER:
            update_powder(sim, x, y);
            break;
        case STATE_LIQUID:
            update_liquid(sim, x, y);
            break;
        case STATE_SOLID:
            if (curr->type >= PARTICLE_DPL_A && curr->type <= PARTICLE_DPL_C) {
                update_dpl(sim, x, y);
            } else {
                update_solid(sim, x, y);
            }
            break;
        case STATE_GAS:
            update_gas(sim, x, y);
            break;
    }
}

void sim_update(Simulation *sim) {
    sim->current_tick++;

    memcpy(sim->grid_next, sim->grid_current, sizeof(Particle) * SIM_WIDTH * SIM_HEIGHT);
    for (int i = 0; i < SIM_WIDTH * SIM_HEIGHT; i++) {
        sim->grid_current[i].handled = false;
        sim->grid_next[i].handled = false;
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

    Particle *temp = sim->grid_current;
    sim->grid_current = sim->grid_next;
    sim->grid_next = temp;
}

void sim_brush_cirlce(Simulation *sim, int cx, int cy, int radius, ParticleType type) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > r2) continue;
            if (type == PARTICLE_SEED && rng_float(sim) > 0.05f) continue;
            if (type == PARTICLE_DPL_SPAWNER) {
                if (rng_float(sim) > 0.15f) continue;
                ParticleType dpl_type = PARTICLE_DPL_A + (rng_xorshift(sim) % 3);
                sim_spawn_particles(sim, cx + dx, cy + dy, dpl_type);
            } else {
                sim_spawn_particles(sim, cx + dx, cy + dy, type);
            }
        }
    }
}

void sim_brush_erase(Simulation *sim, int cx, int cy, int radius) {
    int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > r2) continue;
            sim_remove_particle(sim, cx + dx, cy + dy);
        }
    }
}

void sim_clear(Simulation *sim) {
    memset(sim->grid_current, 0, sizeof(Particle) * SIM_WIDTH * SIM_HEIGHT);
    memset(sim->grid_next, 0, sizeof(Particle) * SIM_WIDTH * SIM_HEIGHT);
}
