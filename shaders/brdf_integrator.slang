import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
RWTexture2D<float2> out_image;

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const float2 uv = (gl_GlobalInvocationID.xy + 0.5) / float2(image_size(out_image).xy);
    out_image[int2(gl_GlobalInvocationID.xy)] = integrate_brdf(uv.x, uv.y);
}

