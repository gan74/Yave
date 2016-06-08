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

#include "Node.h"
#include "ExecutionType.h"
#include "ExecutionFrame.h"
#include "ExecutionException.h"

namespace n {
namespace script {
namespace ast {

void ExprInstruction::eval(ExecutionFrame &frame) const {
	ExecutionVar r = expression->eval(frame);
	r.type->print(r);
}

void InstructionList::eval(ExecutionFrame &frame) const {
	frame.varStack.pushStack();
	for(Instruction *i : instructions) {
		i->eval(frame);
	}
	frame.varStack.popStack();
}

void Declaration::eval(ExecutionFrame &frame) const {
	if(frame.varStack.isDeclared(name)) {
		throw ExecutionException("\"" + name + "\" has already been declared", index);
	}
	core::Map<core::String, ExecutionType *>::iterator it = frame.types.find(typeName);
	if(it == frame.types.end()) {
		throw ExecutionException(typeName + " is not a type", index);
	}
	ExecutionVar &a = frame.varStack.declare(name, it->_2);
	if(value) {
		ExecutionVar v = value->eval(frame);
		if(a.type != v.type) {
			throw ExecutionException("Incompatible type affected to \"" + name + "\"", index);
		}
		a = v;
	}
}

ExecutionVar Identifier::eval(ExecutionFrame &frame) const {
	ExecutionVar e = frame.varStack.getVar(name);
	if(!e.type) {
		throw ExecutionException("\"" + name + "\" has not been declared", index);
	}
	return e;
}

ExecutionVar Integer::eval(ExecutionFrame &frame) const {
	return frame.intType->init(value);
}

ExecutionVar BinOp::eval(ExecutionFrame &frame) const {
	ExecutionVar l = lhs->eval(frame);
	ExecutionVar r = rhs->eval(frame);
	switch(type) {
		case NodeType::Add:
			return l.type->add(l, r);

		case NodeType::Substract:
			return l.type->sub(l, r);

		case NodeType::Multiply:
			return l.type->mul(l, r);

		case NodeType::Divide:
			return l.type->div(l, r);

		default:
		break;
	}
	throw ExecutionException("Unsupported operation", index);

}

ExecutionVar Assignation::eval(ExecutionFrame &frame) const {
	ExecutionVar &e = frame.varStack.getVar(name);
	if(!e.type) {
		throw ExecutionException(name + " has not been declared", index);
	}
	return e = value->eval(frame);
}

}
}
}
