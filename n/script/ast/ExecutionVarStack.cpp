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
#include "ExecutionVarStack.h"
#include "ExecutionException.h"

namespace n {
namespace script {
namespace ast {

ExecutionVarStack::ExecutionVarStack() {
	pushStack();
}

ExecutionVar &ExecutionVarStack::declare(const core::String &name, ExecutionType *type) {
	VarMap::iterator it = vars.insert(name, type->init());
	stack.last().append(it);
	return it->_2;
}

void ExecutionVarStack::pushStack() {
	stack.append(decltype(stack)::Element());
}

void ExecutionVarStack::popStack() {
	for(VarMap::iterator it : stack.last()) {
		vars.remove(it);
	}
	stack.pop();
}

ExecutionVar &ExecutionVarStack::getVar(const core::String &name, uint index) {
	VarMap::iterator it = vars.find(name);
	if(it == vars.end()) {
		throw ExecutionException("\"" + name + "\" has not been declared", index);
	}
	return it->_2;
}

bool ExecutionVarStack::isDeclared(const core::String &name) const {
	return vars.exists(name);
}

}
}
}
