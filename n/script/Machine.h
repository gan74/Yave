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
#ifndef N_CORE_MACHINE_H
#define N_CORE_MACHINE_H

#include "DynamicBytecode.h"
#include <n/core/String.h>

namespace n {
namespace script {

class Machine
{
	public:
		struct RuntimeException : public std::exception
		{
			RuntimeException(const core::String &err) : msg(err) {
			}

			const char *what() const noexcept override {
				return msg.toChar();
			}

			const core::String msg;
		};

		static Nothing typeError(PrimitiveType from, PrimitiveType to) {
			throw RuntimeException(core::String("Conversion from ") + from.name() + " to " + to.name() + " impossible.");
		}

		static Nothing typeError(PrimitiveType from) {
			throw RuntimeException(core::String("Conversion from ") + from.name() + " impossible.");
		}

		static Nothing error() {
			throw RuntimeException("Unknown error");
		}

		static DynamicPrimitive run(DynamicBytecode *code);
	private:
};

}
}

#endif // N_CORE_MACHINE_H
