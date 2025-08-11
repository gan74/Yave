#include "lib/utils.slang"

[[vk::binding(0)]]
RWStructuredBuffer<uint> out_count;

[shader("compute")]
[numthreads(1)]
void comp() {
    out_count[0] = 0;
}


