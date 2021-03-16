#include "lib/utils.glsl"
#include "lib/gbuffer.glsl"
#include "lib/ibl.glsl"
// -------------------------------- DEFINES --------------------------------

#ifdef POINT_LIGHT
#define Light PointLight
#endif

#ifdef SPOT_LIGHT
#define Light SpotLight
#endif

