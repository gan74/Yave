/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/


#ifndef N_SCRIPT_DYNAMICBYTECODE_H
#define N_SCRIPT_DYNAMICBYTECODE_H

#include "Bytecode.h"
#include "DynamicPrimitive.h"
#include <n/defines.h>
#ifndef N_NO_SCRIPT

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

		Ex,


		Expect,
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

#endif

#endif // N_SCRIPT_DYMANIC_BYTECODE_H
