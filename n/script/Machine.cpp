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
#include "Machine.h"

namespace n {
namespace script {

Machine::Machine() : argStackTop(new Primitive[1 << 16]) {
}

Machine::Primitive Machine::run(const BytecodeInstruction *bcode, uint memSize) {
	Primitive *memory = new Primitive[memSize];
	for(uint i = 0; i != memSize; i++) {
		memory[i] = 0;
	}
	Primitive ret;
	run(bcode, memory, &ret);

	delete[] memory;
	return ret;
}

void Machine::run(const BytecodeInstruction *bcode, Primitive *mem, Primitive *ret) {
	Primitive *stackTop = mem;

	for(const BytecodeInstruction *i = bcode;; i++) {

		Primitive *m = mem + i->registers[0];
		Primitive tmp;

		switch(i->op) {

			case Bytecode::Nope:
			break;

			case Bytecode::AddI:
				*m = mem[i->registers[1]] + mem[i->registers[2]];
			break;

			case Bytecode::SubI:
				*m = mem[i->registers[1]] - mem[i->registers[2]];
			break;

			case Bytecode::MulI:
				*m = mem[i->registers[1]] * mem[i->registers[2]];
			break;

			case Bytecode::DivI:
				tmp = mem[i->registers[2]];
				if(!tmp) {
					fatal("divided by zero");
				}
				*m = mem[i->registers[1]] / tmp;
			break;

			case Bytecode::Not:
				*m = !mem[i->registers[1]];
			break;

			case Bytecode::Equals:
				*m = mem[i->registers[1]] == mem[i->registers[2]];
			break;

			case Bytecode::NotEq:
				*m = mem[i->registers[1]] != mem[i->registers[2]];
			break;

			case Bytecode::LessI:
				*m = mem[i->registers[1]] < mem[i->registers[2]];
			break;

			case Bytecode::GreaterI:
				*m = mem[i->registers[1]] > mem[i->registers[2]];
			break;

			case Bytecode::Copy:
				*m = mem[i->registers[1]];
			break;

			case Bytecode::Set:
				*m = i->data();
			break;

			case Bytecode::Jump:
				i = bcode + i->udata();
			break;

			case Bytecode::JumpZ:
				if(!*m) {
					i = bcode + i->udata();
				}
			break;

			case Bytecode::JumpNZ:
				if(*m) {
					i = bcode + i->udata();
				}
			break;

			case Bytecode::FuncHead1:
				i++;
			case Bytecode::FuncHead2:
				stackTop = mem + i->registers[0];
				argStackTop -= i->registers[1];
				memcpy(mem, argStackTop, sizeof(Primitive) * i->registers[1]);
			break;

			case Bytecode::Call:
				run(funcTable[i->udata()], stackTop, m);
			break;

			case Bytecode::PushArg:
				*argStackTop = *m;
				argStackTop++;
			break;

			case Bytecode::Ret:
				*ret = *m;
			case Bytecode::Exit:
				return;

		}
	}
}

void Machine::load(const BytecodeInstruction *bcode, const BytecodeInstruction *end) {
	for(const BytecodeInstruction *i = bcode; i != end; i++) {
		switch(i->op) {
			case Bytecode::FuncHead1:
				while(funcTable.size() <= i->udata()) {
					funcTable << nullptr;
				}
				funcTable[i->udata()] = i;
			break;


			default:
			break;
		}
	}
}

}
}
