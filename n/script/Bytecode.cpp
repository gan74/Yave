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

#include "Bytecode.h"
#include "DynamicBytecode.h"
#include "PrimitieType.h"
#ifndef N_NO_SCRIPT

namespace n {
namespace script {

static_assert(sizeof(Bytecode) == 8, "sizeof(Bytecode) should be 64 bits");
static_assert(sizeof(DynamicBytecode) == 8, "sizeof(DynamicBytecode) should be 64 bits");
static_assert(sizeof(DynamicBytecode) == sizeof(Bytecode), "sizeof(DynamicBytecode) should be equals to sizeof(Bytecode)");

static_assert(sizeof(DynamicPrimitive) == 3 * sizeof(uint16), "sizeof(DynamicPrimitive) should 48 bits");
static_assert(sizeof(PrimitiveType) == 2, "sizeof(PrimitiveType) should be 16 bits");

}
}

#endif
