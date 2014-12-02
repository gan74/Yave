#ifndef N_SCRIPT_BYTECODE_H
#define N_SCRIPT_BYTECODE_H

#include <n/types.h>
#include <n/defines.h>
#include "Primitive.h"

namespace n {
namespace script {

struct Bytecode
{
	enum Type
	{
		Nop,
		End,
		SwMode,

		Max
	};

	Bytecode(Type t) : op(t) {
	}

	const uint16 op;
	uint16 rad;
	union
	{
		Primitive data;
		uint16 ad[2];
	};
};

}
}

#endif // N_SCRIPT_BYTECODE_H
