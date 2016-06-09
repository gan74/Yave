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
#include "WorkTreeBuilder.h"
#include "ASTExpressions.h"
#include "ASTInstructions.h"

namespace n {
namespace script {

const char *ValidationErrorException::what() const noexcept {
	return msg.data();
}

const char *ValidationErrorException::what(const core::String &code) const noexcept {
	buffer = msg + "\n" + position.toString(code);
	return buffer.data();
}



WorkTreeBuilder::WorkTreeBuilder() {
}

WorkTreeBuilder::Guard WorkTreeBuilder::stack() {
	return Guard(this);
}

void WorkTreeBuilder::pushStack() {
	variablesStack.pushStack();
}

void WorkTreeBuilder::popStack() {
	variablesStack.popStack();
}

WorkTreeVariable *WorkTreeBuilder::declareVar(const core::String &name, const core::String &typeName, TokenPosition tk) {
	if(variablesStack.isDeclared(name)) {
		throw ValidationErrorException("\"" + name + "\" has already been declared", tk);
	}
	WorkTreeVariable *v = new WorkTreeVariable(name, 0);
	variables.append(v);
	variablesStack.declare(name, v);
	return v;
}

WorkTreeVariable *WorkTreeBuilder::getVar(const core::String &name, TokenPosition tk) {
	if(!variablesStack.isDeclared(name)) {
		throw ValidationErrorException("\"" + name + "\" has not been declared", tk);
	}
	return variablesStack.get(name);
}














WorkTreeExpression *ASTIdentifier::toWorkTree(WorkTreeBuilder &builder) const {
	return builder.getVar(name, position);
}

WorkTreeExpression *ASTLiteral::toWorkTree(WorkTreeBuilder &) const {
	switch(value.type) {
		case Token::Integer:
			return new WorkTreeInt(value.string.to<int64>());

		default:
		break;
	}
	throw ValidationErrorException("Unsupported literal type", value.position);
	return 0;
}

WorkTreeExpression *ASTBinOp::toWorkTree(WorkTreeBuilder &builder) const {
	WorkTreeExpression *l = lhs->toWorkTree(builder);
	WorkTreeExpression *r = rhs->toWorkTree(builder);
	switch(type) {
		case Token::Plus:
			return new WorkTreeBinOp(WorkTreeNode::Add, l, r);
		case Token::Minus:
			return new WorkTreeBinOp(WorkTreeNode::Substract, l, r);
		case Token::Multiply:
			return new WorkTreeBinOp(WorkTreeNode::Multiply, l, r);
		case Token::Divide:
			return new WorkTreeBinOp(WorkTreeNode::Divide, l, r);

		case Token::Equals:
			return new WorkTreeBinOp(WorkTreeNode::Equals, l, r);
		case Token::NotEquals:
			return new WorkTreeBinOp(WorkTreeNode::NotEquals, l, r);

		default:
		break;
	}
	delete l;
	delete r;
	throw ValidationErrorException("Unsupported operation", position);
	return 0;
}

WorkTreeExpression *ASTAssignation::toWorkTree(WorkTreeBuilder &builder) const {
	WorkTreeVariable *v = builder.getVar(name, position);
	return new WorkTreeAssignation(v, value->toWorkTree(builder));
}

WorkTreeInstruction *ASTInstructionList::toWorkTree(WorkTreeBuilder &builder) const {
	builder.pushStack();
	N_SCOPE(builder.popStack());
	return new WorkTreeBlock(instructions.mapped([&](ASTInstruction *in) { return in->toWorkTree(builder); }));
}

WorkTreeInstruction *ASTDeclaration::toWorkTree(WorkTreeBuilder &builder) const {
	WorkTreeExpression *v = value ? value->toWorkTree(builder) : 0;
	return new WorkTreeDeclaration(builder.declareVar(name, typeName), v);
}

WorkTreeInstruction *ASTLoopInstruction::toWorkTree(WorkTreeBuilder &builder) const {
	WorkTreeExpression *c = condition->toWorkTree(builder);
	return new WorkTreeLoop(c, body->toWorkTree(builder));
}

WorkTreeInstruction *ASTBranchInstruction::toWorkTree(WorkTreeBuilder &builder) const {
	WorkTreeExpression *c = condition->toWorkTree(builder);
	builder.pushStack();
	WorkTreeInstruction *t = thenBody->toWorkTree(builder);
	builder.popStack();
	builder.pushStack();
	WorkTreeInstruction *e = elseBody ? elseBody->toWorkTree(builder) : 0;
	builder.popStack();
	return new WorkTreeBranch(c, t, e);
}

WorkTreeInstruction *ASTExprInstruction::toWorkTree(WorkTreeBuilder &builder) const {
	return new WorkTreeExprInstr(expression->toWorkTree(builder));
}


}
}
