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
#include "WTBuilder.h"
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


WTBuilder::WTBuilder() : types(new WTTypeSystem()) {
}

WTTypeSystem *WTBuilder::getTypeSystem() const {
	return types;
}

void WTBuilder::pushStack() {
	variablesStack.pushStack();
}

void WTBuilder::popStack() {
	variablesStack.popStack();
}

uint WTBuilder::allocRegister() {
	return variablesStack.allocRegister();
}

WTVariable *WTBuilder::declareVar(const core::String &name, const core::String &typeName, TokenPosition tk) {
	if(variablesStack.isDeclared(name)) {
		throw ValidationErrorException("\"" + name + "\" has already been declared", tk);
	}
	WTVariableType *type = types->getType(typeName);
	if(!type) {
		throw ValidationErrorException("\"" + typeName + "\" is not a type", tk);
	}
	WTVariable *v = variablesStack.declare(name, type);
	variables.append(v);
	return v;
}

WTVariable *WTBuilder::getVar(const core::String &name, TokenPosition tk) const {
	WTVariable *v = variablesStack.get(name);
	if(!v) {
		throw ValidationErrorException("\"" + name + "\" has not been declared", tk);
	}
	return v;
}










WTVariableType *notNull(WTVariableType *t, const TokenPosition &p) {
	if(!t) {
		throw ValidationErrorException("Operation on incompatible types", p);
	}
	return t;
}



WTExpression *ASTIdentifier::toWorkTree(WTBuilder &builder, uint) const {
	return builder.getVar(name, position);
}

WTExpression *ASTLiteral::toWorkTree(WTBuilder &builder, uint workReg) const {
	switch(value.type) {
		case Token::Integer:
			return new WTInt(value.string.to<int64>(), builder.getTypeSystem()->getIntType(), workReg);

		default:
		break;
	}
	throw ValidationErrorException("Unsupported literal type", value.position);
	return 0;
}

WTExpression *ASTBinOp::toWorkTree(WTBuilder &builder, uint workReg) const {
	WTExpression *l = lhs->toWorkTree(builder, workReg);
	builder.pushStack();
	WTExpression *r = rhs->toWorkTree(builder, builder.allocRegister());
	builder.popStack();
	switch(type) {
		case Token::Plus:
			return new WTBinOp(WTNode::Add, l, r, notNull(builder.getTypeSystem()->add(l->expressionType, r->expressionType), position), workReg);
		case Token::Minus:
			return new WTBinOp(WTNode::Substract, l, r, builder.getTypeSystem()->getIntType(), workReg);
		case Token::Multiply:
			return new WTBinOp(WTNode::Multiply, l, r, builder.getTypeSystem()->getIntType(), workReg);
		case Token::Divide:
			return new WTBinOp(WTNode::Divide, l, r, builder.getTypeSystem()->getIntType(), workReg);
		case Token::Equals:
			return new WTBinOp(WTNode::Equals, l, r, builder.getTypeSystem()->getIntType(), workReg);
		case Token::NotEquals:
			return new WTBinOp(WTNode::NotEquals, l, r, builder.getTypeSystem()->getIntType(), workReg);

		default:
		break;
	}
	delete l;
	delete r;
	throw ValidationErrorException("Unsupported operation", position);
	return 0;
}

WTExpression *ASTAssignation::toWorkTree(WTBuilder &builder, uint) const {
	WTVariable *v = builder.getVar(name, position);
	WTExpression *val = value->toWorkTree(builder, v->registerIndex);
	if(!builder.getTypeSystem()->assign(v->expressionType, val->expressionType)) {
		throw ValidationErrorException("Assignation of incompatible types", position);
	}
	return new WTAssignation(v, val);
}

WTInstruction *ASTDeclaration::toWorkTree(WTBuilder &builder) const {
	WTVariable *var = builder.declareVar(name, typeName, position);

	builder.pushStack();
	WTExpression *val = value ? value->toWorkTree(builder, var->registerIndex) : new WTInt(0, builder.getTypeSystem()->getType(typeName), var->registerIndex);
	builder.popStack();

	return new WTExprInstr(new WTAssignation(var, val));
}





WTInstruction *ASTInstructionList::toWorkTree(WTBuilder &builder) const {
	builder.pushStack();
	N_SCOPE(builder.popStack());

	return new WTBlock(instructions.mapped([&](ASTInstruction *in) { return in->toWorkTree(builder); }));
}

WTInstruction *ASTLoopInstruction::toWorkTree(WTBuilder &builder) const {
	builder.pushStack();
	WTExpression *c = condition->toWorkTree(builder, builder.allocRegister());
	builder.popStack();

	return new WTLoop(c, body->toWorkTree(builder));
}

WTInstruction *ASTBranchInstruction::toWorkTree(WTBuilder &builder) const {
	builder.pushStack();
	WTExpression *c = condition->toWorkTree(builder, builder.allocRegister());
	builder.popStack();

	builder.pushStack();
	WTInstruction *t = thenBody->toWorkTree(builder);
	builder.popStack();

	builder.pushStack();
	WTInstruction *e = elseBody ? elseBody->toWorkTree(builder) : 0;
	builder.popStack();

	return new WTBranch(c, t, e);
}

WTInstruction *ASTExprInstruction::toWorkTree(WTBuilder &builder) const {
	builder.pushStack();
	N_SCOPE(builder.popStack());

	return new WTExprInstr(expression->toWorkTree(builder, builder.allocRegister()));
}


}
}
