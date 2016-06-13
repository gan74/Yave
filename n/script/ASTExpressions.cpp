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
#include "WTNode.h"
#include "WTBuilder.h"
#include "ASTExpressions.h"

namespace n {
namespace script {

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
	builder.enterScope();
	WTExpression *r = rhs->toWorkTree(builder, builder.allocRegister());
	builder.leaveScope();
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
		case Token::LessThan:
			return new WTBinOp(WTNode::LessThan, l, r, builder.getTypeSystem()->getIntType(), workReg);
		case Token::GreaterThan:
			return new WTBinOp(WTNode::GreaterThan, l, r, builder.getTypeSystem()->getIntType(), workReg);

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

WTExpression *ASTCall::toWorkTree(WTBuilder &builder, uint workReg) const {
	builder.enterScope();
	core::Array<WTExpression *> arg = args.mapped([&](ASTExpression *e) { return e->toWorkTree(builder, builder.allocRegister()); });
	builder.leaveScope();

	#warning function call arguments not checked

	return new WTCall(builder.getFunc(name, position), arg, workReg);
}


/*void ASTIdentifier::resolveFunctions(WTBuilder &builder) const {
}

void ASTLiteral::resolveFunctions(WTBuilder &builder) const {
}

void ASTBinOp::resolveFunctions(WTBuilder &builder) const {
}

void ASTAssignation::resolveFunctions(WTBuilder &builder) const {
}

void ASTCall::resolveFunctions(WTBuilder &builder) const {
}*/

}
}
