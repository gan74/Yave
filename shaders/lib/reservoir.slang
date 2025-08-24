#ifndef RESERVOIR_SLANG
#define RESERVOIR_SLANG

#include "utils.slang"
#include "rng.slang"

// https://interplayoflight.wordpress.com/2023/12/17/a-gentler-introduction-to-restir/
struct Reservoir {
    uint Y; // index of most important light
    float W_y; // light weight
    float W_sum; // sum of all weights for all lights processed
    float M; // number of lights processed for this reservoir

    __init() {
        Y = uint(-1);
        W_y = 0.0;
        W_sum = 0.0;
        M = 0.0;
    }


    [mutating]
    bool update<T: Rng>(uint X, float w, float c, inout T rng) {
        W_sum += w;
        M += c;

        if(rng.next_float() < w / W_sum) {
            Y = X;
            return true;
        }

        return false;
    }


    [mutating]
    float reintegrate_sample(float p_hat) {
        W_y = p_hat > 0.0 ? rcp(p_hat) * W_sum / M : 0.0;
        return W_y;
    }
};


#endif
