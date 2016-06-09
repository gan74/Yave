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
#include <iostream>

namespace n {
namespace script {

const char *SynthaxErrorException::what() const noexcept {
	buffer = "{ ";
	for(TokenType t : expected) {
		buffer << tokenName[uint(t)] << " ";
	}
	buffer = "Expected " + buffer + "} got " + tokenName[uint(position->type)] + " (\"" + position->string + "\")";
	return buffer.data();
}

const char *SynthaxErrorException::what(const core::String &code) const noexcept {
	buffer = "{ ";
	for(TokenType t : expected) {
		buffer << tokenName[uint(t)] << " ";
	}

	uint line = 1;
	for(uint i = 0; i != position->index && i != code.size(); i++) {
		line += code[i] == '\n';
	}
	core::String lineStr("at line ");
	lineStr << core::String2(line) << ": \"";

	buffer = "Expected " + buffer + "} got " + tokenName[uint(position->type)] + ":";
	if(position->type == TokenType::Identifier) {
		buffer += " \"" + position->string + "\"";
	}
	buffer += "\n" + lineStr;

	uint lineBeg = position->index;
	for(; lineBeg != 0 && code[lineBeg - 1] != '\n'; lineBeg--);
	uint end = code.find('\n', lineBeg) - code.begin();
	buffer += code.subString(lineBeg, end - lineBeg) + "\"\n";
	for(uint i = lineBeg; i != position->index + lineStr.size(); i++) {
		buffer += "~";
	}
	buffer += "^";

	return buffer.data();
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

bool isOperator(TokenType type) {
	switch(type) {
		case TokenType::Plus:
		case TokenType::Minus:
		case TokenType::Multiply:
		case TokenType::Divide:
		case TokenType::Equals:
		case TokenType::NotEquals:
			return true;
		break;

		default:
			return false;
	}
	return false;
}

int assoc(TokenType type) {
	switch(type) {
		case TokenType::Multiply:
		case TokenType::Divide:
			return 1;

		case TokenType::Plus:
		case TokenType::Minus:
			return 0;

		default:
			return -1;
	}
	return 0;
}






ast::Expression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end);

ast::Expression *parseSimpleExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	core::Array<Token>::const_iterator id = begin;
	switch((begin++)->type) {
		case TokenType::Identifier:
			if(begin->type == TokenType::Assign) {
				begin++;
				return new ast::Assignation(id->string, parseExpr(begin, end));
			}
			return new ast::Identifier(id->string, id->index);

		case TokenType::Integer:
			return new ast::Integer(id->string.to<int64>(), id->index);

		case TokenType::LeftPar: {
			ast::Expression *expr = parseExpr(begin, end);
			if((begin++)->type != TokenType::RightPar) {
				delete expr;
				expected(begin, TokenType::RightPar);
			}
			return expr;
		} break;

		default:
		break;
	}
	expected(begin, TokenType::Identifier, TokenType::Integer, TokenType::LeftPar);
	return 0;
}

ast::Expression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ast::Expression *lhs = parseSimpleExpr(begin, end);
	TokenType a = begin->type;

	if(!isOperator(a)) {
		return lhs;
	}
	begin++;
	ast::Expression *mhs = parseSimpleExpr(begin, end);

	for(;;) {
		TokenType b = begin->type;

		if(isOperator(b)) {
			begin++;
		} else {
			return new ast::BinOp(lhs, mhs, ast::NodeType(a));
		}

		ast::Expression *rhs = parseSimpleExpr(begin, end);

		if(assoc(b) > assoc(a)) {
			mhs = new ast::BinOp(mhs, rhs, ast::NodeType(b));
		} else {
			lhs = new ast::BinOp(lhs, mhs, ast::NodeType(a));
			mhs = rhs;
			a = b;
		}
	}
	return 0;
}

ast::Declaration *parseDeclaration(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	eat(begin, TokenType::Var);

	expect(begin, TokenType::Identifier);
	core::String name = (begin++)->string;

	eat(begin, TokenType::Colon);

	expect(begin, TokenType::Identifier);
	core::String type = (begin++)->string;

	if(begin->type == TokenType::Assign) {
		begin++;
		return new ast::Declaration(name, type, (begin - 1)->index, parseExpr(begin, end));
	}
	return new ast::Declaration(name, type, (begin - 1)->index);
}


ast::Instruction *parseInstruction(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ast::Instruction *instr = 0;
	switch(begin->type) {
		case TokenType::Var:
			instr = parseDeclaration(begin, end);
		break;

		case TokenType::If: {
			begin++;
			expect(begin, TokenType::LeftPar);
			ast::Expression *expr = parseExpr(begin, end);
			return new ast::BranchInstruction(expr, parseInstruction(begin, end), 0);
		}	break;

		case TokenType::While: {
			begin++;
			expect(begin, TokenType::LeftPar);
			ast::Expression *expr = parseExpr(begin, end);
			return new ast::LoopInstruction(expr, parseInstruction(begin, end));
		} break;

		case TokenType::LeftBrace: {
			begin++;
			core::Array<ast::Instruction *> instrs;
			while(begin->type != TokenType::RightBrace) {
				instrs.append(parseInstruction(begin, end));
			}
			begin++;
			return new ast::InstructionList(instrs);
		} break;

		default:
			instr = new ast::ExprInstruction(parseExpr(begin, end));
		break;
	}
	if(instr) {
		eat(begin, TokenType::SemiColon);
		return instr;
	}
	expected(begin, TokenType::Var, TokenType::While, TokenType::If, TokenType::LeftBrace);
	return 0;
}



Parser::Parser() {
}

ast::Instruction *Parser::parse(core::Array<Token>::const_iterator begin, core::Array<Token>::const_iterator end) const {
	core::Array<ast::Instruction *> instrs;
	while(begin != end && !begin->isEnd()) {
		instrs.append(parseInstruction(begin, end));
	}
	return instrs.size() == 1 ? instrs.first() : new ast::InstructionList(instrs);
}

}
}
