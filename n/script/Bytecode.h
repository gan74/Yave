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


#ifndef N_SCRIPT_BYTECODE_H
#define N_SCRIPT_BYTECODE_H

#include <n/types.h>
#include <n/defines.h>
#include "Primitive.h"
#include <n/defines.h>
#ifndef N_NO_SCRIPT

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

#endif

#endif // N_SCRIPT_BYTECODE_H
