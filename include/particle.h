#ifndef PARTICLE_H_
#define PARTICLE_H_

#include "color.h"

typedef enum {
    PARTICLE_NONE = 0,
    PARTICLE_SAND,
    PARTICLE_WATER
} ParticleType;

typedef struct {
    ParticleType type;
    Color color;
} Particle;

extern const Particle SAND_PARTICLE;
extern const Particle WATER_PARTICLE;

void init_particles();

#endif
