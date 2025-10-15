#include "lib/utils.slang"
#include "lib/std_bindings.slang"

struct VertexStageOut {
    float4 sv_position  : SV_Position;
    uint instance_index;
};



[shader("vertex")]
VertexStageOut vert_main(StdVertexStageIn in) {
    const TransformableData transformable = transformables[mesh_objects[semantics.instance_index].transform_index];

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, mul(transformable.current, float4(in.position, 1.0)));
        out.instance_index = semantics.instance_index;
    }
    return out;
}


[shader("fragment")]
uint frag_main(uint in_instance_index) : SV_Target0 {
    return mesh_objects[in_instance_index].material_index + 1;
}
