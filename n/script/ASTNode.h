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
#ifndef N_SCRIPT_ASTNODE_H
#define N_SCRIPT_ASTNODE_H

#include "Tokenizer.h"
#include "ASTExecutionVar.h"

namespace n {
namespace script {

class ASTExecutionFrame;

enum class ASTNodeType
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

struct ASTNode : NonCopyable
{

	ASTNode(ASTNodeType tpe, uint i = 0) : type(tpe), index(i) {
	}

	const ASTNodeType type;
	const uint index;

	virtual ~ASTNode() {
	}

	virtual core::String toString() const  = 0;
};



struct ASTExpression : public ASTNode
{
	ASTExpression(ASTNodeType tpe, uint index, bool cons = false) : ASTNode(tpe, index), constant(cons) {
	}

	const bool constant;

	virtual ASTExecutionVar eval(ASTExecutionFrame &) const = 0;
};



struct ASTInstruction : public ASTNode
{
	ASTInstruction(ASTNodeType tpe, uint index) : ASTNode(tpe, index) {
	}

	virtual void eval(ASTExecutionFrame &) const = 0;
};



struct ASTExprInstruction : public ASTInstruction
{
	ASTExprInstruction(ASTExpression *expr) : ASTInstruction(ASTNodeType::Expression, expr->index), expression(expr) {
	}

	const ASTExpression *expression;

	virtual core::String toString() const override {
		return expression->toString() + ";";
	}

	virtual void eval(ASTExecutionFrame &frame) const override;
};


struct ASTInstructionList : public ASTInstruction
{
	ASTInstructionList(const core::Array<ASTInstruction *> &instrs) : ASTInstruction(ASTNodeType::InstructionList, instrs.isEmpty() ? 0 : instrs.first()->index), instructions(instrs) {
	}

	const core::Array<ASTInstruction *> instructions;

	virtual core::String toString() const override {
		core::String str;
		for(ASTInstruction *i : instructions) {
			str += i->toString() + "\n";
		}
		return "{\n" + str + "}";
	}

	virtual void eval(ASTExecutionFrame &frame) const override;
};


struct ASTDeclaration : public ASTInstruction
{
	ASTDeclaration(const core::String &n, const core::String &tn, uint index, ASTExpression *val = 0) : ASTInstruction(ASTNodeType::Declaration, index), name(n), typeName(tn), value(val) {
	}

	const core::String name;
	const core::String typeName;
	const ASTExpression *value;

	virtual core::String toString() const override {
		return "var " + name + ":" + typeName + (value ? " = " + value->toString() : core::String());
	}

	virtual void eval(ASTExecutionFrame &frame) const override;
};



struct ASTIdentifier : public ASTExpression
{
	ASTIdentifier(const core::String &n, uint index) : ASTExpression(ASTNodeType::Identifier, false, index), name(n) {
	}

	const core::String name;

	virtual core::String toString() const override {
		return name;
	}

	virtual ASTExecutionVar eval(ASTExecutionFrame &frame) const override;
};



struct ASTInteger : public ASTExpression
{
	ASTInteger(int64 v, uint index) : ASTExpression(ASTNodeType::Integer, true, index), value(v) {
	}

	const int64 value;

	virtual core::String toString() const override {
		return core::String(value);
	}

	virtual ASTExecutionVar eval(ASTExecutionFrame &frame) const override;
};



struct ASTBinOp : public ASTExpression
{
	ASTBinOp(ASTExpression *l, ASTExpression *r, ASTNodeType t) : ASTExpression(t, l->constant && r->constant, l->index), lhs(l), rhs(r) {
	}

	const ASTExpression *lhs;
	const ASTExpression *rhs;

	virtual core::String toString() const override {
		return "(" + lhs->toString() + " "  + core::String(tokenName[uint(type)]) + " " + rhs->toString() + ")";
	}

	virtual ASTExecutionVar eval(ASTExecutionFrame &frame) const override;
};




struct ASTAssignation : public ASTExpression
{
	ASTAssignation(const core::String &id, ASTExpression *val) : ASTExpression(ASTNodeType::Assignation, val->constant, val->index), name(id), value(val) {
	}

	const core::String name;
	const ASTExpression *value;

	virtual core::String toString() const override {
		return name + " = " + value->toString();
	}

	virtual ASTExecutionVar eval(ASTExecutionFrame &frame) const override;
};




}
}

#endif // N_SCRIPT_ASTNODE_H
