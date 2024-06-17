#ifndef STD_BINDINGS_SLANG
#define STD_BINDINGS_SLANG

#include "structs.slang"

[[vk::binding(0, 0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(0, 1)]]
StructuredBuffer<TransformableData> transformables;

[[vk::binding(1, 1)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(2, 1)]]
StructuredBuffer<uint2> transform_material_indices;

#endif
