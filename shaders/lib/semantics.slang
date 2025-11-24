#ifndef SEMANTICS_SLANG
#define SEMANTICS_SLANG

// import "glsl";

typealias Sampler2DShadow = Sampler2D<float>;


namespace semantics {

in float4 frag_coord : SV_Position;
in int instance_index : SV_VulkanInstanceID;
in bool is_front_face : SV_IsFrontFace;
in int vertex_index : SV_VulkanVertexID;

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

property uint work_group_index {
    [__unsafeForceInlineEarly]
    get {
        return
            work_group_id.x +
            work_group_id.y * work_group_count.x +
            work_group_id.z * work_group_count.x * work_group_count.y
        ;
    }
}

property uint work_group_thread_count {
    [__unsafeForceInlineEarly]
    get {
        return work_group_size.x * work_group_size.y * work_group_size.z;
    }
}

property uint3 dispatch_size {
    [__unsafeForceInlineEarly]
    get {
        return work_group_size * work_group_count;
    }
}

property uint3 dispatch_thread_count {
    [__unsafeForceInlineEarly]
    get {
        return dispatch_size.x * dispatch_size.y * dispatch_size.z;
    }
}

property uint global_index {
    [__unsafeForceInlineEarly]
    get {
        return
            global_id.x +
            global_id.y * dispatch_size.x +
            global_id.z * dispatch_size.x * dispatch_size.z
        ;
    }
}

property uint2 pixel_coord {
    [require(compute)]
    get { return global_id.xy; }
    [require(fragment)]
    get { return uint2(frag_coord.xy); }
}

}




[ForceInline]
uint image_mips<T: ITexelElement>(Sampler2D<T> smp) {
    uint mip;
    uint2 size;
    smp.GetDimensions(0, size.x, size.y, mip);
    return 0;
}

[ForceInline]
uint image_mips<T: ITexelElement>(SamplerCube<T> smp) {
    uint mip;
    uint2 size;
    smp.GetDimensions(0, size.x, size.y, mip);
    return 0;
}

[ForceInline]
uint2 image_size<T: ITexelElement>(Sampler2D<T> smp) {
    uint2 size;
    smp.GetDimensions(size.x, size.y);
    return size;
}

[ForceInline]
uint2 image_size<T: ITexelElement>(RWTexture2D<T> smp) {
    uint2 size;
    smp.GetDimensions(size.x, size.y);
    return size;
}

[ForceInline]
uint3 image_size<T: ITexelElement>(RWTexture2DArray<T> smp) {
    uint3 size;
    smp.GetDimensions(size.x, size.y, size.z);
    return size;
}



[ForceInline]
uint3 image_size<T: ITexelElement>(Sampler3D<T> smp) {
    uint3 size;
    smp.GetDimensions(size.x, size.y, size.z);
    return size;
}

[ForceInline]
uint3 image_size<T: ITexelElement>(RWTexture3D<T> smp) {
    uint3 size;
    smp.GetDimensions(size.x, size.y, size.z);
    return size;
}


#endif
