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
#include "ASTInstructions.h"

namespace n {
namespace script {

WTInstruction *ASTDeclaration::toWorkTree(WTBuilder &builder) const {
	WTVariable *var = builder.declareVar(name, typeName, position);

	builder.enterScope();
	WTExpression *val = value ? value->toWorkTree(builder, var->registerIndex) : new WTInt(0, builder.getTypeSystem()->getType(typeName), var->registerIndex);
	builder.leaveScope();

	return new WTExprInstr(new WTAssignation(var, val));
}

WTInstruction *ASTBlock::toWorkTree(WTBuilder &builder) const {
	builder.enterScope();
	core::Array<WTInstruction *> in;
	for(ASTInstruction *i : instructions) {
		WTInstruction *ii = i->toWorkTree(builder);
		if(ii) {
			in.append(ii);
		}
	}
	builder.leaveScope();
	return new WTBlock(in);
}

WTInstruction *ASTLoop::toWorkTree(WTBuilder &builder) const {
	builder.enterScope();
	WTExpression *c = condition->toWorkTree(builder, builder.allocRegister());
	builder.leaveScope();

	return new WTLoop(c, body->toWorkTree(builder));
}

WTInstruction *ASTBranch::toWorkTree(WTBuilder &builder) const {
	builder.enterScope();
	WTExpression *c = condition->toWorkTree(builder, builder.allocRegister());
	builder.leaveScope();

	builder.enterScope();
	WTInstruction *t = thenBody->toWorkTree(builder);
	builder.leaveScope();

	builder.enterScope();
	WTInstruction *e = elseBody ? elseBody->toWorkTree(builder) : 0;
	builder.leaveScope();

	return new WTBranch(c, t, e);
}

WTInstruction *ASTExprInstruction::toWorkTree(WTBuilder &builder) const {
	builder.enterScope();
	N_SCOPE(builder.leaveScope());

	return new WTExprInstr(expression->toWorkTree(builder, builder.allocRegister()));
}

WTInstruction *ASTFunctionDeclaration::toWorkTree(WTBuilder &builder) const {
	WTFunction *function = builder.getFunc(name, position);
	builder.enterFunction(function);
	function->body = body->toWorkTree(builder);
	builder.leaveFunction();
	return 0;
}

WTInstruction *ASTReturn::toWorkTree(WTBuilder &builder) const {
	if(!builder.getCurrentFunction()) {
		throw ValidationErrorException("return statement outside function", position);
	}
	#warning function may not have a return statement
	return new WTReturn(expression->toWorkTree(builder, builder.allocRegister()));
}



void ASTDeclaration::resolveFunctions(WTBuilder &) const {
}

void ASTBlock::resolveFunctions(WTBuilder &builder) const {
	for(ASTInstruction *i : instructions) {
		i->resolveFunctions(builder);
	}
}

void ASTLoop::resolveFunctions(WTBuilder &builder) const {
	body->resolveFunctions(builder);
}

void ASTBranch::resolveFunctions(WTBuilder &builder) const {
	thenBody->resolveFunctions(builder);
	if(elseBody) {
		elseBody->resolveFunctions(builder);
	}
}

void ASTExprInstruction::resolveFunctions(WTBuilder &) const {
}

void ASTFunctionDeclaration::resolveFunctions(WTBuilder &builder) const {
	builder.enterFunction();
	core::Array<WTVariable *> arg;
	for(ASTDeclaration *d : args) {
		if(d->value) {
			throw ValidationErrorException("Function parameter \"" + d->name + "\" should not have a value", position);
		}
		arg.append(builder.declareVar(d->name, d->typeName, d->position));
	}
	builder.leaveFunction();

	#warning function always return int
	builder.declareFunc(name, arg, builder.getTypeSystem()->getIntType(), position);
}

void ASTReturn::resolveFunctions(WTBuilder &) const {
}

}
}
