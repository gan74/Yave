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
#ifndef N_SCRIPT_WTVARIABLESTACK_H
#define N_SCRIPT_WTVARIABLESTACK_H

#include "WTNode.h"
#include <n/core/Map.h>

namespace n {
namespace script {

class WTVariableStack
{
	using VMap = core::Map<core::String, WTVariable *>;

	struct StackData
	{
		core::Array<typename VMap::iterator> vars;
		uint registerIndex;
	};

	public:
		WTVariableStack();

		WTVariable *declare(const core::String &name, WTVariableType *type);

		WTVariable *get(const core::String &name) const;
		bool isDeclared(const core::String &name) const;

		void pushStack();
		void popStack();

	private:

		uint getRegisterIndex();

		VMap variables;
		core::Array<StackData> stack;
};

}
}

#endif // N_SCRIPT_WTVARIABLESTACK_H
