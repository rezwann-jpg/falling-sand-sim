#include "particle.h"
#include "color.h"
#include <stdbool.h>

static unsigned int rng_next(unsigned int *state) {
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static float rng_float(unsigned int *state) {
    return (float)(rng_next(state) % 10000) / 10000.0f;
}

static const ParticleProperties PARTICLE_PROPERTIES[PARTICLE_COUNT] = {
    {
        .name = "Empty",
        .state = STATE_GAS,
        .density = 0.0f,
        .lifetime = -1
    },
    {
        .name = "Sand",
        .state = STATE_POWDER,
        .density = 1600.0f,
        .friction = 0.7f,
        .bounciness = 0.1f,
        .viscosity = 0.0f,
        .ignition_point = 1,
        .flammability = 0.0f,
        .lifetime = -1,
        .burns_into = PARTICLE_NONE
    },
    {
        .name = "Water",
        .state = STATE_LIQUID,
        .density = 1000.0f,
        .friction = 0.1f,
        .bounciness = 0.0f,
        .viscosity = 0.1f,
        .ignition_point = -1.0f,
        .flammability = 0.0f,
        .lifetime = -1,
        .burns_into = PARTICLE_NONE
    },
    {
        .name = "Stone",
        .state = STATE_SOLID,
        .density = 2700.0f,
        .viscosity = 0.0f,
        .friction = 0.9f,
        .bounciness = 0.2f,
        .ignition_point = -1.0f,
        .flammability = 0.0f,
        .lifetime = -1,
        .burns_into = PARTICLE_NONE
    },
    {
        .name = "Wood",
        .state = STATE_SOLID,
        .density = 600.0f,
        .friction = 0.8f,
        .bounciness = 0.15f,
        .viscosity = 0.0f,
        .ignition_point = 60.0f,
        .flammability = 0.6f,
        .lifetime = -1,
        .burns_into = PARTICLE_FIRE,
    },
    {
        .name = "Fire",
        .state = STATE_GAS,
        .density = 0.3f,
        .friction = 0.0f,
        .bounciness = 0.0f,
        .viscosity = 0.1f,
        .ignition_point = -1.0f,
        .flammability = 0.0f,
        .lifetime = 60,
        .color_min = COLOR_FIRE_MIN,
        .color_max = COLOR_FIRE,
        .burns_into = PARTICLE_NONE
    },
    {
        .name = "Smoke",
        .state = STATE_GAS,
        .density = 0.5f,
        .friction = 0.0f,
        .bounciness = 0.0f,
        .viscosity = 0.05f,
        .ignition_point = -1.0f,
        .flammability = 0.0f,
        .lifetime = 200,
        .burns_into = PARTICLE_NONE,
    },
    {
        .name = "Steam",
        .state = STATE_GAS,
        .density = 0.6f,
        .friction = 0.0f,
        .bounciness = 0.0f,
        .viscosity = 0.02f,
        .ignition_point = -1.0f,
        .flammability = 0.0f,
        .lifetime = 300,
        .burns_into = PARTICLE_NONE,
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

Particle particle_create(ParticleType type, unsigned int *rng_state) {
    const ParticleProperties *props = particles_get_properties(type);

    Particle p = { 0 };
    p.type = type;
    p.vx = 0;
    p.vy = 0;
    p.active = true;
    p.last_updated_tick = -1;
    p.temperature = AMBIENT_TEMP;
    p.lifetime = props->lifetime;
    p.burning = false;

    p.render_color = particles_get_packed_color(type);

    if (type == PARTICLE_FIRE) {
        p.temperature = 600.0f + rng_float(rng_state) * 400.0f;
    }

    return p;
}

static Uint32 PACKED_COLORS[PARTICLE_COUNT];

void particles_init_colors() {
    Color colors[PARTICLE_COUNT] = {
        COLOR_AIR,
        COLOR_SAND,
        COLOR_WATER,
        COLOR_STONE,
        COLOR_WOOD,
        COLOR_FIRE,
        COLOR_SMOKE,
        COLOR_STEAM
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
