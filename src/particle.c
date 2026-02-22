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
    },
    {
        .name = "Stone",
        .state = STATE_SOLID,
        .density = 2700.0f,
        .viscosity = 0.0f
    }
};

const ParticleProperties* particles_get_properties(ParticleType type) {
    if (type >= PARTICLE_COUNT)
        return &PARTICLE_PROPERTIES[PARTICLE_NONE];

    return &PARTICLE_PROPERTIES[type];
}

// Color get_color(ParticleType type) {
//     switch (type) {
//         case PARTICLE_NONE:
//             return COLOR_AIR;
//         case PARTICLE_SAND:
//             return COLOR_SAND;
//         case PARTICLE_WATER:
//             return COLOR_WATER;
//         default:
//             return COLOR_AIR;
//     }
// }

// Particle particle_create(ParticleType type) {
//     // const ParticleProperties *props = particles_get_properties(type);

//     Particle p = { 0 };
//     p.type = type;
//     p.vx = 0.0f;
//     p.vy = 0.0f;
//     p.color = get_color(type);
//     p.active = true;

//     return p;
// }

static Uint32 PACKED_COLORS[PARTICLE_COUNT];

void particles_init_colors() {
    Color colors[PARTICLE_COUNT] = {
        COLOR_AIR,
        COLOR_SAND,
        COLOR_WATER,
        COLOR_STONE
    };

    for (int i = 0; i < PARTICLE_COUNT; i++) {
        PACKED_COLORS[i] =
            (colors[i].a << 24) |
            (colors[i].b << 16) |
            (colors[i].g << 8)  |
            (colors[i].r);
    }
}

Uint32 particles_get_packed_color(ParticleType type) {
    return PACKED_COLORS[type];
}
