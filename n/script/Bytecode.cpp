#include "Bytecode.h"
#include "DynamicBytecode.h"
#include "PrimitieType.h"

namespace n {
namespace script {

static_assert(sizeof(Bytecode) == 8, "sizeof(Bytecode) should be 64 bits");
static_assert(sizeof(DynamicBytecode) == 8, "sizeof(DynamicBytecode) should be 64 bits");
static_assert(sizeof(DynamicBytecode) == sizeof(Bytecode), "sizeof(DynamicBytecode) should be equals to sizeof(Bytecode)");

static_assert(sizeof(DynamicPrimitive) == 3 * sizeof(uint16), "sizeof(DynamicPrimitive) should 48 bits");
static_assert(sizeof(PrimitiveType) == 2, "sizeof(PrimitiveType) should be 16 bits");

}
}
