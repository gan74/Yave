#include "lib/utils.slang"

[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
RWTexture3D<uint> out_grid;

[[vk::binding(4)]]
cbuffer Params {
    uint frame_id;
};



int round_toward_normal(float x, float nx) {
    if(nx > 0.0) {
        return int(ceil(x));
    }
    return int(floor(x));
}


int3 round_toward_normal(float3 x, float3 nx) {
    return int3(
        round_toward_normal(x.x, nx.x),
        round_toward_normal(x.y, nx.y),
        round_toward_normal(x.z, nx.z)
    );
}


[shader("compute")]
[numthreads(8, 8)]
void comp() {
    const uint2 size = image_size(in_depth);
    const uint2 coord = semantics.global_id.xy;

    if(any(coord >= size)) {
        return;
    }

    const float2 px_center = float2(coord + 0.5);
    const float2 in_uv = px_center / float2(size);

    const float depth = in_depth[coord];
    const SurfaceInfo surface = decode_gbuffer({
        float4(0.0),
        in_rt1[coord],
    });

    const float3 world_pos = unproject(in_uv, depth, camera.curr.inv_view_proj);
    const int3 pos = (round_toward_normal(world_pos, surface.normal) - int3(camera.position)) + int3(32);

    out_grid[pos] = frame_id;

}


