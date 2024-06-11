[require(spirv)]
namespace semantics {

[require(compute)]
static const int3 workgroup_count = spirv_asm { result: $$int3 = OpLoad builtin(NumWorkgroups: int3); };

[require(compute)]
static const int3 global_thead_id = spirv_asm { result: $$int3 = OpLoad builtin(GlobalInvocationId: int3); };



}
