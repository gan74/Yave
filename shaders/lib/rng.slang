#ifndef RNG_SLANG
#define RNG_SLANG

#include "utils.slang"

interface Rng {
    [mutating]
    uint next_u32();

    [mutating]
    float next_float();
};

struct HashRng : Rng {
    uint state;

    __init(uint st) {
        state = st;
    }

    [mutating]
    uint next_u32() {
        state = hash(state);
        return state;
    }

    [mutating]
    float next_float() {
        return (next_u32() & 0x0FFFFFFF) / float(0x0FFFFFFF);
    }
};


#endif // RNG_SLANG
