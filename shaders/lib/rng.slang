#ifndef RNG_SLANG
#define RNG_SLANG

#include "hash.slang"

interface Rng {
    [mutating]
    uint next();

    [mutating]
    float next_unorm() {
        return float(next() & 0xFFFFFF) / float(0xFFFFFF);
    }

    [mutating]
    float next_snorm() {
        return next_unorm() * 2.0 - 1.0;
    }

    [mutating]
    float3 next_unit() {
        const float theta = next_unorm() * constants.pi * 2.0;
        const float z = next_snorm();
        const float n = sqrt(1.0 - z * z);
        return float3(n * cos(theta), n * sin(theta), z);
    }
};

struct HashRng : Rng {
    uint seed;

    __init(uint s) {
        seed = s;
    }

    [mutating]
    uint next() {
        seed += 1;
        return hash.pcg(seed);
    }
}


#endif // RNG_SLANG
