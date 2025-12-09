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
        return normalize(float3(next_snorm(), next_snorm(), next_snorm()));
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
