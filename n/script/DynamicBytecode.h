#ifndef N_SCRIPT_DYNAMICBYTECODE_H
#define N_SCRIPT_DYNAMICBYTECODE_H

#include "Bytecode.h"
#include "DynamicPrimitive.h"

namespace n {
namespace script {

struct DynamicBytecode
{
	enum Type
	{
		Nop,
		End,
		SwMode,

		Push,
		Pop,

		Rot2,
		Rot3,

		Cpy,
		CpyN,

		Add,
		Mul,
		Sub,
		Div,

		BitNot,
		Not,
		Or,
		And,
		Xor,

		SlB,
		SlE,


		Cast,

		Max
	};

	DynamicBytecode(Type t, DynamicPrimitive dat) : op(t) {
		new(raw) DynamicPrimitive(dat);
	}

	DynamicBytecode(const DynamicBytecode &bc) : DynamicBytecode(Type(bc.op), bc.data) {
	}

	DynamicBytecode(Type t) : DynamicBytecode(t, 0) {
	}

	DynamicBytecode(Type t, int i) : DynamicBytecode(t, DynamicPrimitive(PrimitiveType::Int, i)) {
	}

	DynamicBytecode(Type t, float f) : DynamicBytecode(t, DynamicPrimitive(PrimitiveType::Float, f)) {
	}


	const uint16 op;
	union
	{
		DynamicPrimitive data;
		uint16 raw[3];
	};
};

}
}

#endif // N_SCRIPT_DYMANIC_BYTECODE_H
