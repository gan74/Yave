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
#ifndef N_SCRIPT_ASTEXPRESSIONS_H
#define N_SCRIPT_ASTEXPRESSIONS_H

#include "ASTNode.h"
#include "Tokenizer.h"

namespace n {
namespace script {

struct ASTIdentifier : public ASTExpression
{
	ASTIdentifier(const core::String &n, const TokenPosition &pos) : ASTExpression(pos), name(n) {
	}

	const core::String name;

	virtual core::String toString() const override {
		return name;
	}

	virtual WTExpression *toWorkTree(WTBuilder &builder, uint) const override;
	//virtual void resolveFunctions(WTBuilder &) const override;
};

struct ASTLiteral : public ASTExpression
{
	ASTLiteral(const Token &val) : ASTExpression(val.position), value(val) {
	}

	const Token value;

	virtual core::String toString() const override {
		return value.string;
	}

	virtual WTExpression *toWorkTree(WTBuilder &builder, uint workReg) const override;
	//virtual void resolveFunctions(WTBuilder &) const override;
};

struct ASTBinOp : public ASTExpression
{
	ASTBinOp(ASTExpression *l, ASTExpression *r, Token::Type t) : ASTExpression(l->position), type(t), lhs(l), rhs(r) {
	}

	Token::Type type;
	const ASTExpression *lhs;
	const ASTExpression *rhs;

	virtual core::String toString() const override {
		return "(" + lhs->toString() + " "  + Token::getName(type) + " " + rhs->toString() + ")";
	}

	virtual WTExpression *toWorkTree(WTBuilder &builder, uint workReg) const override;
	//virtual void resolveFunctions(WTBuilder &builder) const override;
};

struct ASTAssignation : public ASTExpression
{
	ASTAssignation(const core::String &id, ASTExpression *val, const TokenPosition &tk) : ASTExpression(tk), name(id), value(val) {
	}

	const core::String name;
	const ASTExpression *value;

	virtual core::String toString() const override {
		return name + " = " + value->toString();
	}

	virtual WTExpression *toWorkTree(WTBuilder &builder, uint) const override;
	//virtual void resolveFunctions(WTBuilder &builder) const override;
};

struct ASTCall : public ASTExpression
{
	ASTCall(const core::String &id, const core::Array<ASTExpression *> &args, const TokenPosition &tk) : ASTExpression(tk), name(id), args(args) {
	}

	const core::String name;
	const core::Array<ASTExpression *> args;

	virtual core::String toString() const override {
		core::String a;
		for(ASTExpression *e : args) {
			a += e->toString() + " ";
		}
		return name + "( " + a + ")";
	}

	virtual WTExpression *toWorkTree(WTBuilder &builder, uint workReg) const override;
	//virtual void resolveFunctions(WTBuilder &builder) const override;
};

}
}

#endif // N_SCRIPT_ASTEXPRESSIONS_H
