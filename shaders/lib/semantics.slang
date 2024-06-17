#ifndef SEMANTICS_SLANG
#define SEMANTICS_SLANG

// import "glsl";

typealias Sampler2DShadow = Sampler2D<float>;


namespace semantics {

in float4 frag_coord : SV_Position;
in uint instance_index : SV_InstanceID;

in uint3 global_id : SV_DispatchThreadID;
in uint3 work_group_id : SV_GroupID;
in uint local_index : SV_GroupIndex;
in uint3 local_id : SV_GroupThreadID;

property uint3 work_group_size {
    [__unsafeForceInlineEarly]
    [require(glsl_spirv, GLSL_430_SPIRV_1_0_compute)]
    get { return WorkgroupSize(); }
}

property uint3 work_group_count {
    [require(spirv, GLSL_430_SPIRV_1_0_compute)]
    get { return spirv_asm { result: $$uint3 = OpLoad builtin(NumWorkgroups: uint3); }; }
}

}




uint image_mips<T>(Sampler2D<T> smp) {
    uint mip;
    uint2 size;
    smp.GetDimensions(0, size.x, size.y, mip);
    return 0;
}

uint image_mips<T>(SamplerCube<T> smp) {
    uint mip;
    uint2 size;
    smp.GetDimensions(0, size.x, size.y, mip);
    return 0;
}

uint2 image_size<T>(Sampler2D<T> smp) {
    uint2 size;
    smp.GetDimensions(size.x, size.y);
    return size;
}

uint2 image_size<T>(RWTexture2D<T> smp) {
    uint2 size;
    smp.GetDimensions(size.x, size.y);
    return size;
}

uint3 image_size<T>(RWTexture2DArray<T> smp) {
    uint3 size;
    smp.GetDimensions(size.x, size.y, size.z);
    return size;
}





#endif
