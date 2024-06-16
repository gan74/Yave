#include "lib/utils.slang"

[[vk::binding(0)]]
RWStructuredBuffer<Camera> camera;

[[vk::binding(1)]]
ConstantBuffer<Camera> prev_camera;

[shader("compute")]
[numthreads(1)]
void comp_main() {
    camera[0].prev_unjittered_view_proj = prev_camera.unjittered_view_proj;
}

