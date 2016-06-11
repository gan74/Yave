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

Machine::Machine() {
}

int *Machine::run(const BytecodeInstruction restrict *bcode) {
	 int restrict *mem = new int[1 << 16];
	for(uint i = 0; i != 1 << 16; i++) {
		mem[i] = 0;
	}

	for(const restrict BytecodeInstruction *i = bcode;; i++) {
		int *m = mem + i->registers[0];
		int tmp;

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
					fatal("divide by zero");
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
				i = bcode + i->data();
			break;

			case Bytecode::JumpZ:
				if(!*m) {
					i = bcode + i->data();
				}
			break;

			case Bytecode::JumpNZ:
				if(*m) {
					i = bcode + i->data();
				}
			break;

			case Bytecode::Exit:
				return mem;

		}
	}

	return mem;

}

}
}
