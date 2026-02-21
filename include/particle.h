#ifndef PARTICLE_H_
#define PARTICLE_H_

#include "color.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

typedef enum {
    STATE_SOLID,
    STATE_POWDER,
    STATE_LIQUID,
    STATE_GAS
} ParticleState;

typedef enum {
    PARTICLE_NONE = 0,
    PARTICLE_SAND,
    PARTICLE_WATER,
    PARTICLE_COUNT
} ParticleType;

typedef struct {
    const char *name;
    ParticleState state;
    float density;
    float viscosity;
} ParticleProperties;

typedef struct {
    ParticleType type;
    Color color;
    Uint32 render_color;

    float vx;
    float vy;

    bool active;
    int last_updated_tick;
} Particle;

const ParticleProperties* particles_get_properties(ParticleType type);

void particles_init_colors();
Uint32 particles_get_packed_color(ParticleType type);

#endif
