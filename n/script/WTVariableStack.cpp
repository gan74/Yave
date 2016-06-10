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
#include "WTVariableStack.h"

namespace n {
namespace script {

WTVariableStack::WTVariableStack() {
	StackData data;
	data.registerIndex = 0;
	stack.append(data);
}

WTVariable *WTVariableStack::declare(const core::String &name, WTVariableType *type) {
	typename VMap::iterator it = variables.insert(name, new WTVariable(name, type, getRegisterIndex()));
	stack.last().vars.append(it);
	return it->_2;
}

WTVariable *WTVariableStack::get(const core::String &name) const {
	typename VMap::const_iterator it = variables.find(name);
	return it == variables.end() ? 0 : it->_2;
}

bool WTVariableStack::isDeclared(const core::String &name) const {
	return variables.exists(name);
}

void WTVariableStack::pushStack() {
	StackData data;
	data.registerIndex = stack.last().registerIndex;
	stack.append(data);
}

void WTVariableStack::popStack() {
	for(typename VMap::iterator it : stack.last().vars) {
		variables.remove(it);
	}
	stack.pop();
}

uint WTVariableStack::getRegisterIndex() {
	return stack.last().registerIndex++;
}

}
}
