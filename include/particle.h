#ifndef PARTICLE_H_
#define PARTICLE_H_

#include "color.h"
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

    float vx;
    float vy;

    bool updated;
} Particle;

const ParticleProperties* particles_get_properties(ParticleType type);

Particle particle_create(ParticleType type);

extern const Particle SAND_PARTICLE;
extern const Particle WATER_PARTICLE;
extern const Particle AIR_PARTICLE;

void init_particles();

#endif
