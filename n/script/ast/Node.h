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
#ifndef N_SCRIPT_NODE_H
#define N_SCRIPT_NODE_H

#include <n/script/Tokenizer.h>
#include "ExecutionVar.h"

namespace n {
namespace script {
namespace ast {

class ExecutionFrame;

enum class NodeType
{
	Identifier,

	Integer,
	Float,

	Add			= TokenType::Plus,
	Substract	= TokenType::Minus,
	Multiply	= TokenType::Multiply,
	Divide		= TokenType::Divide,

	Declaration,
	Assignation,

	Expression,
	InstructionList

};

struct Node : NonCopyable
{

	Node(NodeType tpe, uint i = 0) : type(tpe), index(i) {
	}

	const NodeType type;
	const uint index;

	virtual ~Node() {
	}

	virtual core::String toString() const  = 0;
};



struct Expression : public Node
{
	Expression(NodeType tpe, uint index, bool cons = false) : Node(tpe, index), constant(cons) {
	}

	const bool constant;

	virtual ExecutionVar eval(ExecutionFrame &) const = 0;
};



struct Instruction : public Node
{
	Instruction(NodeType tpe, uint index) : Node(tpe, index) {
	}

	virtual void eval(ExecutionFrame &) const = 0;
};



struct ExprInstruction : public Instruction
{
	ExprInstruction(Expression *expr) : Instruction(NodeType::Expression, expr->index), expression(expr) {
	}

	const Expression *expression;

	virtual core::String toString() const override {
		return expression->toString() + ";";
	}

	virtual void eval(ExecutionFrame &frame) const override;
};


struct InstructionList : public Instruction
{
	InstructionList(const core::Array<Instruction *> &instrs) : Instruction(NodeType::InstructionList, instrs.isEmpty() ? 0 : instrs.first()->index), instructions(instrs) {
	}

	const core::Array<Instruction *> instructions;

	virtual core::String toString() const override {
		core::String str;
		for(Instruction *i : instructions) {
			str += i->toString() + "\n";
		}
		return "{\n" + str + "}";
	}

	virtual void eval(ExecutionFrame &frame) const override;
};


struct Declaration : public Instruction
{
	Declaration(const core::String &n, const core::String &tn, uint index, Expression *val = 0) : Instruction(NodeType::Declaration, index), name(n), typeName(tn), value(val) {
	}

	const core::String name;
	const core::String typeName;
	const Expression *value;

	virtual core::String toString() const override {
		return "var " + name + ":" + typeName + (value ? " = " + value->toString() : core::String());
	}

	virtual void eval(ExecutionFrame &frame) const override;
};



struct Identifier : public Expression
{
	Identifier(const core::String &n, uint index) : Expression(NodeType::Identifier, false, index), name(n) {
	}

	const core::String name;

	virtual core::String toString() const override {
		return name;
	}

	virtual ExecutionVar eval(ExecutionFrame &frame) const override;
};



struct Integer : public Expression
{
	Integer(int64 v, uint index) : Expression(NodeType::Integer, true, index), value(v) {
	}

	const int64 value;

	virtual core::String toString() const override {
		return core::String(value);
	}

	virtual ExecutionVar eval(ExecutionFrame &frame) const override;
};



struct BinOp : public Expression
{
	BinOp(Expression *l, Expression *r, NodeType t) : Expression(t, l->constant && r->constant, l->index), lhs(l), rhs(r) {
	}

	const Expression *lhs;
	const Expression *rhs;

	virtual core::String toString() const override {
		return "(" + lhs->toString() + " "  + core::String(tokenName[uint(type)]) + " " + rhs->toString() + ")";
	}

	virtual ExecutionVar eval(ExecutionFrame &frame) const override;
};




struct Assignation : public Expression
{
	Assignation(const core::String &id, Expression *val) : Expression(NodeType::Assignation, val->constant, val->index), name(id), value(val) {
	}

	const core::String name;
	const Expression *value;

	virtual core::String toString() const override {
		return name + " = " + value->toString();
	}

	virtual ExecutionVar eval(ExecutionFrame &frame) const override;
};


}
}
}

#endif // N_SCRIPT_NODE_H
