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

namespace n {
namespace script {

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

	ASTNode(ASTNodeType tpe) : type(tpe) {
	}

	const ASTNodeType type;

	virtual ~ASTNode() {
	}

	virtual core::String toString() const  = 0;
};



struct ASTExpression : public ASTNode
{
	ASTExpression(ASTNodeType tpe, bool cons = false) : ASTNode(tpe), constant(cons) {
	}

	const bool constant;
};



struct ASTInstruction : public ASTNode
{
	ASTInstruction(ASTNodeType tpe) : ASTNode(tpe) {
	}
};



struct ASTExprInstruction : public ASTInstruction
{
	ASTExprInstruction(ASTExpression *expr) : ASTInstruction(ASTNodeType::Expression), expression(expr) {
	}

	const ASTExpression *expression;

	virtual core::String toString() const override {
		return expression->toString() + ";";
	}
};


struct ASTInstructionList : public ASTInstruction
{
	ASTInstructionList(const core::Array<ASTInstruction *> &instrs) : ASTInstruction(ASTNodeType::InstructionList), instructions(instrs) {
	}

	const core::Array<ASTInstruction *> instructions;

	virtual core::String toString() const override {
		core::String str;
		for(ASTInstruction *i : instructions) {
			str += i->toString() + "\n";
		}
		return "{\n" + str + "}";
	}
};


struct ASTDeclaration : public ASTInstruction
{
	ASTDeclaration(const core::String &n, const core::String &tn, ASTExpression *val = 0) : ASTInstruction(ASTNodeType::Declaration), name(n), typeName(tn), value(val) {
	}

	const core::String name;
	const core::String typeName;
	const ASTExpression *value;

	virtual core::String toString() const override {
		return "var " + name + ":" + typeName + (value ? " = " + value->toString() : core::String());
	}
};



struct ASTIdentifier : public ASTExpression
{
	ASTIdentifier(const core::String &n) : ASTExpression(ASTNodeType::Identifier), name(n) {
	}

	const core::String name;

	virtual core::String toString() const override {
		return name;
	}
};



struct ASTInteger : public ASTExpression
{
	ASTInteger(const core::String &v) : ASTExpression(ASTNodeType::Integer, true), value(v) {
	}

	const core::String value;

	virtual core::String toString() const override {
		return value;
	}
};



struct ASTBinOp : public ASTExpression
{
	ASTBinOp(ASTExpression *l, ASTExpression *r, ASTNodeType t) : ASTExpression(t, l->constant && r->constant), lhs(l), rhs(r) {
	}

	const ASTExpression *lhs;
	const ASTExpression *rhs;

	virtual core::String toString() const override {
		return core::String(tokenName[uint(type)]) + "(" + lhs->toString() + " " + rhs->toString() + ")";
	}
};




struct ASTAssignation : public ASTExpression
{
	ASTAssignation(const core::String &id, ASTExpression *val) : ASTExpression(ASTNodeType::Assignation, val->constant), name(id), value(val) {
	}

	const core::String name;
	const ASTExpression *value;

	virtual core::String toString() const override {
		return name + " = " + value->toString();
	}
};




}
}

#endif // N_SCRIPT_ASTNODE_H
