#ifndef STD_BINDINGS_SLANG
#define STD_BINDINGS_SLANG

#include "structs.slang"

[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
StructuredBuffer<TransformableData> transformables;

[[vk::binding(2)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(3)]]
StructuredBuffer<StaticMeshData> mesh_datas;

[[vk::binding(4)]]
StructuredBuffer<MeshObject> mesh_objects;



#endif
