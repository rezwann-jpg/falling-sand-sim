#include "particle.h"
#include "color.h"

static const ParticleProperties PARTICLE_PROPERTIES[PARTICLE_COUNT] = {
    {
        .name = "Empty",
        .state = STATE_GAS,
        .density = 0.0f
    },
    {
        .name = "Sand",
        .state = STATE_POWDER,
        .density = 1600.0f,
        .viscosity = 0.0f
    },
    {
        .name = "Water",
        .state = STATE_LIQUID,
        .density = 1000.0f,
        .viscosity = 0.1f
    }
};

const ParticleProperties* particles_get_properties(ParticleType type) {
    if (type >= PARTICLE_COUNT)
        return &PARTICLE_PROPERTIES[PARTICLE_NONE];

    return &PARTICLE_PROPERTIES[type];
}

Color get_color(ParticleType type) {
    switch (type) {
        case PARTICLE_NONE:
            return COLOR_AIR;
        case PARTICLE_SAND:
            return COLOR_SAND;
        case PARTICLE_WATER:
            return COLOR_WATER;
        default:
            return COLOR_AIR;
    }
}

Particle particle_create(ParticleType type) {
    // const ParticleProperties *props = particles_get_properties(type);

    Particle p = { 0 };
    p.type = type;
    p.vx = 0.0f;
    p.vy = 0.0f;
    p.color = get_color(type);
    p.updated = false;

    return p;
}
