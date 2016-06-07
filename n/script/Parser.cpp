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
		str << tokenName[uint(t)] << " ";
	}
	return ("Expected " + str + "} got " + tokenName[uint(position->type)] + " (\"" + position->string + "\")").data();
}

const char *SynthaxErrorException::what(const core::String &code) const noexcept {
	core::String str = "{ ";
	for(TokenType t : expected) {
		str << tokenName[uint(t)] << " ";
	}

	uint line = 1;
	for(uint i = 0; i != position->index && i != code.size(); i++) {
		line += code[i] == '\n';
	}
	core::String lineStr("at line ");
	lineStr << core::String2(line) << ": \"";

	str = "Expected " + str + "} got " + tokenName[uint(position->type)] + ":";
	if(position->type == TokenType::Identifier) {
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


template<typename... Args>
void expected(core::Array<Token>::const_iterator it, Args... args) {
	throw SynthaxErrorException({args...}, --it);
}

void expect(core::Array<Token>::const_iterator it, TokenType type) {
	if(it->type != type) {
		it++;
		expected(it, type);
	}
}

void eat(core::Array<Token>::const_iterator &it, TokenType type) {
	if((it++)->type != type) {
		expected(it, type);
	}
}


/*
template<typename... Args>
bool eatHelper(TokenType t, TokenType type, Args... args) {
	return t == type ? true : eatHelper(t, args...);
}

bool eatHelper(TokenType) {
	return false;
}

template<typename... Args>
void eat(core::Array<Token>::const_iterator &begin, Args... args) {
	if(!eatHelper((begin++)->type, args...)) {
		expected(begin, args...);
	}
}*/

ASTExpression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end);


ASTExpression *parseSimpleExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ASTExpression *expr = 0;
	core::Array<Token>::const_iterator id = begin;
	switch((begin++)->type) {
		case TokenType::Identifier:
			if(begin->type == TokenType::Assign) {
				begin++;
				return new ASTAssignation(id->string, parseExpr(begin, end));
			}
			return new ASTIdentifier(id->string, id->index);

		case TokenType::Integer:
			return new ASTInteger(id->string.to<int64>(), id->index);

		case TokenType::LeftPar:
			expr = parseExpr(begin, end);
			if((begin++)->type != TokenType::RightPar) {
				delete expr;
				expected(begin, TokenType::RightPar);
			}
			return expr;
		break;

		default:
		break;
	}
	expected(begin, TokenType::Identifier, TokenType::Integer, TokenType::LeftPar);
	return 0;
}

ASTExpression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ASTExpression *expr = parseSimpleExpr(begin, end);
	for(;;) {

		TokenType token = (begin++)->type;
		switch(token) {
			case TokenType::Plus:
			case TokenType::Minus:
				expr = new ASTBinOp(expr, parseExpr(begin, end), ASTNodeType(token));
			break;

			case TokenType::Multiply:
			case TokenType::Divide:
				expr = new ASTBinOp(expr, parseSimpleExpr(begin, end), ASTNodeType(token));
			break;

			default:
				begin--;
				return expr;
		}
	}
	return expr;
}


ASTDeclaration *parseDeclaration(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	eat(begin, TokenType::Var);

	expect(begin, TokenType::Identifier);
	core::String name = (begin++)->string;

	eat(begin, TokenType::Colon);

	expect(begin, TokenType::Identifier);
	core::String type = (begin++)->string;

	if(begin->type == TokenType::Assign) {
		begin++;
		return new ASTDeclaration(name, type, (begin - 1)->index, parseExpr(begin, end));
	}
	return new ASTDeclaration(name, type, (begin - 1)->index);
}


ASTInstruction *parseInstrution(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ASTInstruction *instr = 0;
	switch(begin->type) {
		case TokenType::Var:
			instr = parseDeclaration(begin, end);
		break;

		default:
			instr = new ASTExprInstruction(parseExpr(begin, end));
		break;
	}
	if(instr) {
		eat(begin, TokenType::SemiColon);
		return instr;
	}
	expected(begin, TokenType::Var);
	return 0;
}

Parser::Parser() {
}

ASTInstruction *Parser::parse(core::Array<Token>::const_iterator begin, core::Array<Token>::const_iterator end) const {
	core::Array<ASTInstruction *> instrs;
	while(begin != end && !begin->isEnd()) {
		instrs.append(parseInstrution(begin, end));
	}
	return new ASTInstructionList(instrs);
}

}
}
