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
#ifndef N_SCRIPT_WORKTREENODE_H
#define N_SCRIPT_WORKTREENODE_H

#include <n/core/String.h>
#include <n/core/Array.h>

namespace n {
namespace script {

struct WorkTreeType
{
};

struct WorkTreeNode
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

	WorkTreeNode(Type t) : type(t) {
	}

	const Type type;
};

struct WorkTreeExpression : public WorkTreeNode
{
	WorkTreeExpression(WorkTreeNode::Type t) : WorkTreeNode(t) {
	}
};

struct WorkTreeInstruction : public WorkTreeNode
{
	WorkTreeInstruction(WorkTreeNode::Type t) : WorkTreeNode(t) {
	}
};

struct WorkTreeBinOp : public WorkTreeExpression
{
	WorkTreeBinOp(WorkTreeNode::Type t, WorkTreeExpression *l, WorkTreeExpression *r) : WorkTreeExpression(t), lhs(l), rhs(r) {
	}

	WorkTreeExpression *lhs;
	WorkTreeExpression *rhs;
};


struct WorkTreeVariable : public WorkTreeExpression
{
	WorkTreeVariable(const core::String &n, WorkTreeType *t) : WorkTreeExpression(Variable), name(n), varType(t) {
	}

	core::String name;
	WorkTreeType *varType;
};

struct WorkTreeInt : public WorkTreeExpression
{
	WorkTreeInt(int64 val) : WorkTreeExpression(Integer), value(val) {
	}

	int64 value;
};

struct WorkTreeAssignation : public WorkTreeExpression
{
	WorkTreeAssignation(WorkTreeVariable *var, WorkTreeExpression *val) : WorkTreeExpression(Assignation), variable(var), value(val) {
	}

	WorkTreeVariable *variable;
	WorkTreeExpression *value;
};

struct WorkTreeDeclaration : public WorkTreeInstruction
{
	WorkTreeDeclaration(WorkTreeVariable *var, WorkTreeExpression *val) : WorkTreeInstruction(Declaration), variable(var), value(val) {
	}

	WorkTreeVariable *variable;
	WorkTreeExpression *value;
};

struct WorkTreeLoop : public WorkTreeInstruction
{
	WorkTreeLoop(WorkTreeExpression *cond, WorkTreeInstruction *bod) : WorkTreeInstruction(Loop), condition(cond), body(bod) {
	}

	WorkTreeExpression *condition;
	WorkTreeInstruction *body;
};

struct WorkTreeBranch : public WorkTreeInstruction
{
	WorkTreeBranch(WorkTreeExpression *cond, WorkTreeInstruction *thenBod, WorkTreeInstruction *elseBod) : WorkTreeInstruction(Branch), condition(cond), thenBody(thenBod), elseBody(elseBod) {
	}

	WorkTreeExpression *condition;
	WorkTreeInstruction *thenBody;
	WorkTreeInstruction *elseBody;
};

struct WorkTreeExprInstr : public WorkTreeInstruction
{
	WorkTreeExprInstr(WorkTreeExpression *e) : WorkTreeInstruction(Expression), expression(e) {
	}

	WorkTreeExpression *expression;
};


struct WorkTreeBlock : public WorkTreeInstruction
{
	WorkTreeBlock(const core::Array<WorkTreeInstruction *> &i) : WorkTreeInstruction(Block), instructions(i) {
	}

	core::Array<WorkTreeInstruction *> instructions;
};


}
}

#endif // N_SCRIPT_WORKTREENODE_H
