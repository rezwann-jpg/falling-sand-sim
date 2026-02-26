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
    PARTICLE_STONE,
    PARTICLE_WOOD,
    PARTICLE_FIRE,
    PARTICLE_SMOKE,
    PARTICLE_STEAM,
    PARTICLE_COUNT
} ParticleType;

typedef struct {
    const char *name;
    ParticleState state;
    float density;
    float friction;
    float bounciness;
    float viscosity;
    float boiling_point;
    float ignition_point;
    float thermal_conductivity;
    float flammability;
    int lifetime;
    Color color_min;
    Color color_max;
    ParticleType boils_into;
    ParticleType burns_into;
} ParticleProperties;

typedef struct {
    ParticleType type;
    Color color;
    Uint32 render_color;

    float vx;
    float vy;
    float temperature;
    int lifetime;

    bool active;
    int last_updated_tick;
    bool burning;
} Particle;

Particle particle_create(ParticleType type, unsigned int *rng_state);

const ParticleProperties* particles_get_properties(ParticleType type);

void particles_init_colors();
Uint32 particles_get_packed_color(ParticleType type);

#endif
