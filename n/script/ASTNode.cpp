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

#include "ASTNode.h"
#include "ASTExecutionType.h"
#include "ASTExecutionFrame.h"

namespace n {
namespace script {

void ASTExprInstruction::eval(ASTExecutionFrame &frame) const {
	ASTExecutionVar r = expression->eval(frame);
	r.type->print(r);
}

void ASTInstructionList::eval(ASTExecutionFrame &frame) const {
	frame.varStack.pushStack();
	for(ASTInstruction *i : instructions) {
		i->eval(frame);
	}
	frame.varStack.popStack();
}

void ASTDeclaration::eval(ASTExecutionFrame &frame) const {
	if(frame.varStack.isDeclared(name)) {
		throw ASTExecutionException("\"" + name + "\" has already been declared", index);
	}
	core::Map<core::String, ASTExecutionType *>::iterator it = frame.types.find(typeName);
	if(it == frame.types.end()) {
		throw ASTExecutionException(typeName + " is not a type", index);
	}
	ASTExecutionVar &a = frame.varStack.declare(name, it->_2);
	if(value) {
		ASTExecutionVar v = value->eval(frame);
		if(a.type != v.type) {
			throw ASTExecutionException("Incompatible type affected to \"" + name + "\"", index);
		}
		a = v;
	}
}

ASTExecutionVar ASTIdentifier::eval(ASTExecutionFrame &frame) const {
	ASTExecutionVar e = frame.varStack.getVar(name);
	if(!e.type) {
		throw ASTExecutionException("\"" + name + "\" has not been declared", index);
	}
	return e;
}

ASTExecutionVar ASTInteger::eval(ASTExecutionFrame &frame) const {
	return frame.intType->init(value);
}

ASTExecutionVar ASTBinOp::eval(ASTExecutionFrame &frame) const {
	ASTExecutionVar l = lhs->eval(frame);
	ASTExecutionVar r = rhs->eval(frame);
	if(l.type != frame.intType || r.type != frame.intType || type != ASTNodeType::Add) {
		throw ASTExecutionException("Unsupported operation", index);
	}
	return frame.intType->init(l.integer + r.integer);
}

ASTExecutionVar ASTAssignation::eval(ASTExecutionFrame &frame) const {
	ASTExecutionVar &e = frame.varStack.getVar(name);
	if(!e.type) {
		throw ASTExecutionException(name + " has not been declared", index);
	}
	return e = value->eval(frame);
}

}
}
