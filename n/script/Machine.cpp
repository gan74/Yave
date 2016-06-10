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


int *Machine::run(const BytecodeInstruction *bcode) {
	int *mem = new int[1 << 16];
	for(uint i = 0; i != 1 << 16; i++) {
		mem[i] = 0;
	}

	for(const BytecodeInstruction *i = bcode;; i++) {
		int *m = mem + i->registers[0];

		switch(i->op) {

			case Bytecode::Nope:
			break;

			case Bytecode::AddI:
				*m = mem[i->registers[1]] + mem[i->registers[2]];
			break;

			case Bytecode::MulI:
				*m = mem[i->registers[1]] * mem[i->registers[2]];
			break;

			case Bytecode::Copy:
				*m = mem[i->registers[1]];
			break;

			case Bytecode::Set:
				*m = i->data();
			break;

			case Bytecode::Jump:
				bcode = bcode + i->registers[0] - 1;
			break;

			case Bytecode::JumpNZ:
				if(*m) {
					bcode = bcode + i->registers[1] - 1;
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
