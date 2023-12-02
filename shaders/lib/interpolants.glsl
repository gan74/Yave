
#define DECLARE_STANDARD_INTERPOLANTS(inout_state)                                      \
layout(location = 0) inout_state flat uint inout_state ## _instance_index;              \
layout(location = 1) inout_state vec2 inout_state ## _uv;                               \
layout(location = 2) inout_state vec3 inout_state ## _normal;                           \
layout(location = 3) inout_state vec3 inout_state ## _tangent;                          \
layout(location = 4) inout_state vec3 inout_state ## _bitangent;                        \
layout(location = 5) inout_state vec3 inout_state ## _screen_pos;                       \
layout(location = 6) inout_state vec3 inout_state ## _last_screen_pos;
