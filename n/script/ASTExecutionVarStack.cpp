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
#include "ASTExecutionVarStack.h"

namespace n {
namespace script {

ASTExecutionVarStack::ASTExecutionVarStack() {
	pushStack();
}

ASTExecutionVar &ASTExecutionVarStack::declare(const core::String &name, ASTExecutionType *type) {
	VarMap::iterator it = vars.insert(name, type->init());
	stack.last().append(it);
	return it->_2;
}

void ASTExecutionVarStack::pushStack() {
	stack.append(decltype(stack)::Element());
}

void ASTExecutionVarStack::popStack() {
	for(VarMap::iterator it : stack.last()) {
		vars.remove(it);
	}
	stack.pop();
}

ASTExecutionVar &ASTExecutionVarStack::getVar(const core::String &name) {
	VarMap::iterator it = vars.find(name);
	return it == vars.end() ? invalid : it->_2;
}

bool ASTExecutionVarStack::isDeclared(const core::String &name) const {
	return vars.exists(name);
}


}
}
