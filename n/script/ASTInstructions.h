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
#include "WorkTreeNode.h"
#include "WorkTreeBuilder.h"
#include <n/core/Array.h>

namespace n {
namespace script {

struct ASTInstructionList : public ASTInstruction
{
	ASTInstructionList(const core::Array<ASTInstruction *> &instrs) : ASTInstruction(instrs.isEmpty() ? TokenPosition() : instrs.first()->position), instructions(instrs) {
	}

	const core::Array<ASTInstruction *> instructions;

	virtual core::String toString() const override {
		core::String str;
		for(ASTInstruction *i : instructions) {
			str += i->toString() + "\n";
		}
		return "{\n" + str + "}";
	}

	virtual WorkTreeInstruction *toWorkTree(WorkTreeBuilder &builder) const;
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

	virtual WorkTreeInstruction *toWorkTree(WorkTreeBuilder &builder) const;
};

struct ASTLoopInstruction : public ASTInstruction
{
	ASTLoopInstruction(ASTExpression *cond, ASTInstruction *bod) : ASTInstruction(cond->position), condition(cond), body(bod) {
	}

	const ASTExpression *condition;
	const ASTInstruction *body;

	virtual core::String toString() const override {
		return "while(" + condition->toString() + ") " + body->toString();
	}

	virtual WorkTreeInstruction *toWorkTree(WorkTreeBuilder &builder) const;
};

struct ASTBranchInstruction : public ASTInstruction
{
	ASTBranchInstruction(ASTExpression *cond, ASTInstruction *then, ASTInstruction *el) : ASTInstruction(cond->position), condition(cond), thenBody(then), elseBody(el) {
	}

	const ASTExpression *condition;
	const ASTInstruction *thenBody;
	const ASTInstruction *elseBody;

	virtual core::String toString() const override {
		return "if(" + condition->toString() + ") " + thenBody->toString() + (elseBody ? " else " + elseBody->toString() : "");
	}

	virtual WorkTreeInstruction *toWorkTree(WorkTreeBuilder &builder) const;
};

struct ASTExprInstruction : public ASTInstruction
{
	ASTExprInstruction(ASTExpression *expr) : ASTExprInstruction(expr, !dynamic_cast<ASTAssignation *>(expr)) {
	}

	ASTExprInstruction(ASTExpression *expr, bool prt) : ASTInstruction(expr->position), print(prt), expression(expr) {
	}

	const bool print;
	const ASTExpression *expression;

	virtual core::String toString() const override {
		return expression->toString() + ";";
	}

	virtual WorkTreeInstruction *toWorkTree(WorkTreeBuilder &builder) const;
};

}
}

#endif // N_SCRIPT_ASTINSTRUCTIONS_H
