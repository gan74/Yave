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
#ifndef N_SCRIPT_PARSER_H
#define N_SCRIPT_PARSER_H

#include "Tokenizer.h"

namespace n {
namespace script {


struct ASTNode : NonCopyable
{
	virtual ~ASTNode() {
	}

	virtual core::String toString() const  = 0;
};

struct ASTExpression : public ASTNode {};

struct ASTIdentifier : public ASTExpression
{
	ASTIdentifier(const core::String &n) : name(n) {
	}

	const core::String name;

	virtual core::String toString() const override {
		return name;
	}
};

struct ASTBinOp : public ASTExpression
{
	ASTBinOp(ASTExpression *l, ASTExpression *r, TokenType t) : type(t), lhs(l), rhs(r) {
	}

	const TokenType type;
	const ASTExpression *lhs;
	const ASTExpression *rhs;

	virtual core::String toString() const override {
		return core::String(tokenName[type]) + "(" + lhs->toString() + " " + rhs->toString() + ")";
	}
};

class SynthaxErrorException : public std::exception
{
	public:
		SynthaxErrorException(const core::Array<TokenType> &e, core::Array<Token>::const_iterator p) : expected(e), position(p) {
		}

		virtual const char *what() const noexcept override;
		virtual const char *what(const core::String &code) const noexcept;

	private:
		core::Array<TokenType> expected;
		core::Array<Token>::const_iterator position;
};

class Parser
{
	public:
		Parser();

		ASTNode *parse(core::Array<Token>::const_iterator begin, core::Array<Token>::const_iterator end) const;

};

}
}

#endif // N_SCRIPT_PARSER_H
