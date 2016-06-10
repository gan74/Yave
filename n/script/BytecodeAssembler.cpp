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
#include "BytecodeAssembler.h"

namespace n {
namespace script {

using BCI = BytecodeInstruction;

template<typename T>
BytecodeInstruction::RegisterType tr(T i) {
	return i;
}


BytecodeAssembler::BytecodeAssembler() {
}

BytecodeAssembler::Label BytecodeAssembler::createLabel() {
	return Label(in.size());
}

BytecodeAssembler &BytecodeAssembler::ass(BytecodeInstruction i) {
	in << i;
	return *this;
}

BytecodeAssembler &BytecodeAssembler::operator<<(BytecodeInstruction i) {
	return ass(i);
}

BytecodeAssembler &BytecodeAssembler::operator<<(const BytecodeAssembler &a) {
	uint s = in.size();
	in.setMinCapacity(s + a.in.size());
	for(BytecodeInstruction i : a.in) {
		if(i.op == Bytecode::Jump) {
			i.registers[0] += s;
		} else if(i.op == Bytecode::JumpNZ) {
			i.registers[1] += s;
		}
		in << i;
	}
	return *this;
}








BytecodeAssembler &BytecodeAssembler::addI(RegisterType to, RegisterType a, RegisterType b) {
	return ass(BCI{Bytecode::AddI, tr(to), tr(a), tr(b)});
}

BytecodeAssembler &BytecodeAssembler::mulI(RegisterType to, RegisterType a, RegisterType b) {
	return ass(BCI{Bytecode::MulI, tr(to), tr(a), tr(b)});
}

BytecodeAssembler &BytecodeAssembler::set(RegisterType to, int64 value) {
	if(value > std::numeric_limits<BytecodeInstruction::DataType>::max() || value < std::numeric_limits<BytecodeInstruction::DataType>::min()) {
		#warning BytecodeAssembler fatal
		fatal("BytecodeAssembler : value too big");
	}
	BCI i{Bytecode::Set, tr(to)};
	i.data() = value;
	return ass(i);
}

BytecodeAssembler &BytecodeAssembler::copy(RegisterType to, RegisterType from) {
	return ass(BCI{Bytecode::Copy, tr(to), tr(from)});
}

BytecodeAssembler &BytecodeAssembler::jump(Label to) {
	return ass(BCI{Bytecode::Jump, tr(to.index)});
}

BytecodeAssembler &BytecodeAssembler::jumpNZ(RegisterType a, Label to) {
	return ass(BCI{Bytecode::JumpNZ, tr(a), tr(to.index)});
}

BytecodeAssembler &BytecodeAssembler::exit() {
	return ass(BCI{Bytecode::Exit, {0}});
}

}
}
