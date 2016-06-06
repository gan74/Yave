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
#include "Parser.h"

namespace n {
namespace script {

const char *SynthaxErrorException::what() const noexcept {
	core::String str = "{ ";
	for(TokenType t : expected) {
		str << tokenName[t] << " ";
	}
	return ("Expected " + str + "} got " + tokenName[position->type] + " (\"" + position->string + "\")").data();
}

const char *SynthaxErrorException::what(const core::String &code) const noexcept {
	core::String str = "{ ";
	for(TokenType t : expected) {
		str << tokenName[t] << " ";
	}

	uint line = 1;
	for(uint i = 0; i != position->index && i != code.size(); i++) {
		line += code[i] == '\n';
	}
	core::String lineStr("at line ");
	lineStr << core::String2(line) << ": \"";

	str = "Expected " + str + "} got " + tokenName[position->type] + ":";
	if(position->type == Identifier) {
		str += " \"" + position->string + "\"";
	}
	str += "\n" + lineStr;

	uint lineBeg = position->index;
	for(; lineBeg != 0 && code[lineBeg - 1] != '\n'; lineBeg--);
	uint end = code.find('\n', lineBeg) - code.begin();
	str += code.subString(lineBeg, end - lineBeg) + "\"\n";
	for(uint i = lineBeg; i != position->index + lineStr.size(); i++) {
		str += "~";
	}
	str += "^";

	return str.data();
}


void expected(core::Array<Token>::const_iterator it, const core::Array<TokenType> &expected) {
	throw SynthaxErrorException(expected, --it);
}

void expect(TokenType type, core::Array<Token>::const_iterator &begin) {
	if((begin++)->type != type) {
		expected(begin, {type});
	}
}







ASTExpression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end);


ASTExpression *parseSimpleExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ASTExpression *expr = 0;
	core::Array<Token>::const_iterator id = begin;
	switch((begin++)->type) {
		case Identifier:
			return new ASTIdentifier(id->string);

		case LeftPar:
			expr = parseExpr(begin, end);
			if((begin++)->type != RightPar) {
				delete expr;
				expected(begin, {RightPar});
			}
			return expr;
		break;

		default:
		break;
	}
	expected(begin, {Identifier, LeftPar});
	return 0;
}

ASTExpression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ASTExpression *expr = parseSimpleExpr(begin, end);
	for(;;) {
		ASTExpression *rhs = 0;
		TokenType token = (begin++)->type;
		switch(token) {
			case Plus:
			case Minus:
				rhs = parseExpr(begin, end);
				if(rhs) {
					expr = new ASTBinOp(expr, rhs, token);
				}
			break;

			case Multiply:
			case Divide:
				rhs = parseSimpleExpr(begin, end);
				if(rhs) {
					expr = new ASTBinOp(expr, rhs, token);
				}
			break;

			default:
				begin--;
				/*delete expr;
				expected(begin, {Plus, Minus, Multiply, Divide});*/
				return expr;
		}
	}
	return expr;
}




Parser::Parser() {
}

ASTNode *Parser::parse(core::Array<Token>::const_iterator begin, core::Array<Token>::const_iterator end) const {
	ASTNode *node =  parseExpr(begin, end);
	/*if(begin != end) {
		expected(begin, {End});
	}*/
	return node;
}

}
}
