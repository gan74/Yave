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

namespace n {
namespace script {

enum Bytecode : uint16
{
	Nope,

	AddI,
	SubI,
	MulI,
	DivI,

	Not,

	Equals,
	NotEq,

	LessI,
	GreaterI,

	Copy,

	Set,

	Jump,
	JumpZ,
	JumpNZ,

	FuncHead1, //  func id
	FuncHead2, // arg num, stack size

	Call,
	PushArg,
	Ret,

	Exit

};

struct BytecodeInstruction
{
	using RegisterType = uint16;
	using DataType = int32;

	using UnsignedDataType = std::make_unsigned<DataType>::type;

	Bytecode op;
	uint16 registers[3];

	DataType &data() {
		return *(reinterpret_cast<DataType *>(this) + 1);
	}

	const DataType &data() const {
		return *(reinterpret_cast<const DataType *>(this) + 1);
	}

	UnsignedDataType &udata() {
		return *(reinterpret_cast<UnsignedDataType *>(this) + 1);
	}

	const UnsignedDataType &udata() const {
		return *(reinterpret_cast<const UnsignedDataType *>(this) + 1);
	}
};

static_assert(sizeof(BytecodeInstruction::RegisterType) + sizeof(Bytecode) == sizeof(BytecodeInstruction::DataType), "BytecodeInstruction DataType should be 2 * RegisterType");
static_assert(sizeof(BytecodeInstruction) == 8, "BytecodeInstruction should be 64 bits");

}
}
#endif // N_SCRIPT_BYTECODE_H
