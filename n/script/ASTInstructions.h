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
#ifndef N_SCRIPT_ASTINSTRUCTIONS_H
#define N_SCRIPT_ASTINSTRUCTIONS_H

#include "ASTNode.h"
#include "WTNode.h"
#include "WTBuilder.h"
#include <n/core/Array.h>

namespace n {
namespace script {

struct ASTBlock : public ASTInstruction
{
	ASTBlock(const core::Array<ASTInstruction *> &instrs) : ASTInstruction(instrs.isEmpty() ? TokenPosition() : instrs.first()->position), instructions(instrs) {
	}

	const core::Array<ASTInstruction *> instructions;

	virtual core::String toString() const override {
		core::String str;
		for(ASTInstruction *i : instructions) {
			str += i->toString() + "\n";
		}
		return "{\n" + str + "}";
	}

	virtual WTInstruction *toWorkTree(WTBuilder &builder) const override;
	virtual void resolveFunctions(WTBuilder &builder) const override;
};

struct ASTDeclaration : public ASTInstruction
{
	ASTDeclaration(const core::String &n, const core::String &tn, const TokenPosition &pos, ASTExpression *val = 0) : ASTInstruction(pos), name(n), typeName(tn), value(val) {
	}

	const core::String name;
	const core::String typeName;
	const ASTExpression *value;

	virtual core::String toString() const override {
		return "var " + name + ":" + typeName + (value ? " = " + value->toString() : core::String()) + ";";
	}

	virtual WTInstruction *toWorkTree(WTBuilder &builder) const override;
	virtual void resolveFunctions(WTBuilder &) const override;
};

struct ASTLoop : public ASTInstruction
{
	ASTLoop(ASTExpression *cond, ASTInstruction *bod) : ASTInstruction(cond->position), condition(cond), body(bod) {
	}

	const ASTExpression *condition;
	const ASTInstruction *body;

	virtual core::String toString() const override {
		return "while(" + condition->toString() + ") " + body->toString();
	}

	virtual WTInstruction *toWorkTree(WTBuilder &builder) const override;
	virtual void resolveFunctions(WTBuilder &builder) const override;
};

struct ASTBranch : public ASTInstruction
{
	ASTBranch(ASTExpression *cond, ASTInstruction *then, ASTInstruction *el) : ASTInstruction(cond->position), condition(cond), thenBody(then), elseBody(el) {
	}

	const ASTExpression *condition;
	const ASTInstruction *thenBody;
	const ASTInstruction *elseBody;

	virtual core::String toString() const override {
		return "if(" + condition->toString() + ") " + thenBody->toString() + (elseBody ? " else " + elseBody->toString() : "");
	}

	virtual WTInstruction *toWorkTree(WTBuilder &builder) const override;
	virtual void resolveFunctions(WTBuilder &builder) const override;
};

struct ASTExprInstruction : public ASTInstruction
{
	ASTExprInstruction(ASTExpression *expr) : ASTInstruction(expr->position), expression(expr) {
	}

	const ASTExpression *expression;

	virtual core::String toString() const override {
		return expression->toString() + ";";
	}

	virtual WTInstruction *toWorkTree(WTBuilder &builder) const override;
	virtual void resolveFunctions(WTBuilder &) const override;
};

struct ASTFunctionDeclaration : public ASTInstruction
{
	ASTFunctionDeclaration(const core::String &n, const core::Array<ASTDeclaration *> &a, ASTInstruction *bod) : ASTInstruction(bod->position), name(n), args(a), body(bod) {
	}

	const core::String name;
	const core::Array<ASTDeclaration *> args;
	const ASTInstruction *body;

	virtual core::String toString() const override {
		core::String a;
		for(ASTDeclaration *d : args) {
			a += d->toString() + " ";
		}
		return "def " + name + "( " + a + ") = " + body->toString();
	}

	virtual WTInstruction *toWorkTree(WTBuilder &builder) const override;
	virtual void resolveFunctions(WTBuilder &builder) const override;
};

struct ASTReturn : public ASTInstruction
{
	ASTReturn(ASTExpression *expr) : ASTInstruction(expr->position), expression(expr) {
	}

	const ASTExpression *expression;

	virtual core::String toString() const override {
		return "return " + expression->toString() + ";";
	}

	virtual WTInstruction *toWorkTree(WTBuilder &builder) const override;
	virtual void resolveFunctions(WTBuilder &) const override;
};

}
}

#endif // N_SCRIPT_ASTINSTRUCTIONS_H
