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
#ifndef N_SCRIPT_WTNODE_H
#define N_SCRIPT_WTNODE_H

#include <n/core/String.h>
#include <n/core/Array.h>
#include "WTVariableType.h"

namespace n {
namespace script {

struct WTNode : NonCopyable
{
	enum Type
	{
		Add,
		Substract,
		Multiply,
		Divide,

		Equals,
		NotEquals,

		Variable,

		Integer,

		Assignation,
		Declaration,

		Loop,
		Branch,

		Expression,
		Block
	};

	WTNode(Type t) : type(t) {
	}

	const Type type;
};

struct WTExpression : public WTNode
{
	WTExpression(WTNode::Type t, WTVariableType *vt) : WTNode(t), expressionType(vt) {
	}

	WTVariableType *expressionType;
};

struct WTInstruction : public WTNode
{
	WTInstruction(WTNode::Type t) : WTNode(t) {
	}
};

struct WTBinOp : public WTExpression
{
	WTBinOp(WTNode::Type t, WTExpression *l, WTExpression *r, WTVariableType *ty) : WTExpression(t, ty), lhs(l), rhs(r) {
	}

	WTExpression *lhs;
	WTExpression *rhs;
};


struct WTVariable : public WTExpression
{
	WTVariable(const core::String &n, WTVariableType *t, uint reg) : WTExpression(Variable, t), name(n), registerIndex(reg) {
	}

	core::String name;
	const uint registerIndex;
};

struct WTInt : public WTExpression
{
	WTInt(int64 val, WTVariableType *intType) : WTExpression(Integer, intType), value(val) {
	}

	int64 value;
};

struct WTAssignation : public WTExpression
{
	WTAssignation(WTVariable *var, WTExpression *val) : WTExpression(Assignation, var->expressionType), variable(var), value(val) {
	}

	WTVariable *variable;
	WTExpression *value;
};

struct WTDeclaration : public WTInstruction
{
	WTDeclaration(WTVariable *var, WTExpression *val) : WTInstruction(Declaration), variable(var), value(val) {
	}

	WTVariable *variable;
	WTExpression *value;
};

struct WTLoop : public WTInstruction
{
	WTLoop(WTExpression *cond, WTInstruction *bod) : WTInstruction(Loop), condition(cond), body(bod) {
	}

	WTExpression *condition;
	WTInstruction *body;
};

struct WTBranch : public WTInstruction
{
	WTBranch(WTExpression *cond, WTInstruction *thenBod, WTInstruction *elseBod) : WTInstruction(Branch), condition(cond), thenBody(thenBod), elseBody(elseBod) {
	}

	WTExpression *condition;
	WTInstruction *thenBody;
	WTInstruction *elseBody;
};

struct WTExprInstr : public WTInstruction
{
	WTExprInstr(WTExpression *e) : WTInstruction(Expression), expression(e) {
	}

	WTExpression *expression;
};


struct WTBlock : public WTInstruction
{
	WTBlock(const core::Array<WTInstruction *> &i) : WTInstruction(Block), instructions(i) {
	}

	core::Array<WTInstruction *> instructions;
};


}
}

#endif // N_SCRIPT_WTNODE_H
