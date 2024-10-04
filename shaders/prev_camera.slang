#include "lib/utils.slang"

[[vk::binding(0)]]
RWStructuredBuffer<Camera> camera;

[[vk::binding(1)]]
ConstantBuffer<Camera> prev_camera;

[shader("compute")]
[numthreads(1)]
void comp_main() {
    camera[0].prev = prev_camera.cur;
    // camera[0].prev_to_cur = mul(camera[0].cur.view_proj, camera[0].prev.inv_view_proj);
}

