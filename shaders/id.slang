#include "lib/utils.slang"
#include "lib/std_bindings.slang"


[[vk::binding(0, 1)]]
StructuredBuffer<uint> attrib_buffers[];


struct VertexStageOut {
    float4 sv_position  : SV_Position;
    uint instance_index;
};


[shader("vertex")]
VertexStageOut vert_main() {
    const MeshObject mesh = mesh_objects[semantics.instance_index];
    const StdVertexData vert = read_vertex(attrib_buffers, semantics.vertex_index, mesh_datas[mesh.mesh_data_index]);
    const TransformableData transformable = transformables[mesh.transform_index];

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, mul(transformable.current, float4(vert.position, 1.0)));
        out.instance_index = semantics.instance_index;
    }
    return out;
}


[shader("fragment")]
uint frag_main(uint in_instance_index) : SV_Target0 {
    return mesh_objects[in_instance_index].material_index + 1;
}
