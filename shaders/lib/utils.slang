[require(compute, spirv)]
int3 workgroup_count() {
    return spirv_asm {
        result: $$int3 = OpLoad builtin(NumWorkgroups: int3);
    };
}
