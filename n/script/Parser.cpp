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
#include "ASTExpressions.h"
#include "ASTInstructions.h"

namespace n {
namespace script {

const char *SynthaxErrorException::what() const noexcept {
	buffer = "{ ";
	for(Token::Type t : expected) {
		buffer << Token::getName(t) << " ";
	}
	buffer = "Expected " + buffer + "} got " + Token::getName(token.type) + " (\"" + token.string + "\")";
	return buffer.data();
}

const char *SynthaxErrorException::what(const core::String &code) const noexcept {
	buffer = "{ ";
	for(Token::Type t : expected) {
		buffer << Token::getName(t) << " ";
	}
	buffer = "Expected " + buffer + "} got " + Token::getName(token.type) + ":";
	if(token.type == Token::Identifier) {
		buffer += " \"" + token.string + "\"";
	}
	buffer += "\n" + token.position.toString(code);

	return buffer.data();
}




template<typename... Args>
void expected(core::Array<Token>::const_iterator it, Args... args) {
	throw SynthaxErrorException({args...}, *(--it));
}

void expect(core::Array<Token>::const_iterator it, Token::Type type) {
	if(it->type != type) {
		it++;
		expected(it, type);
	}
}

void eat(core::Array<Token>::const_iterator &it, Token::Type type) {
	if((it++)->type != type) {
		expected(it, type);
	}
}

bool isOperator(Token::Type type) {
	return !!(type & Token::isOperator);
}

uint assoc(Token::Type type) {
	return type & Token::associativityMask;
}






ASTExpression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end);

ASTExpression *parseSimpleExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	core::Array<Token>::const_iterator id = begin;
	switch((begin++)->type) {
		case Token::Identifier:
			if(begin->type == Token::Assign) {
				begin++;
				return new ASTAssignation(id->string, parseExpr(begin, end));
			}
			return new ASTIdentifier(id->string, id->position);

		case Token::Integer:
			return new ASTLiteral(*id);

		case Token::LeftPar: {
			ASTExpression *expr = parseExpr(begin, end);
			if((begin++)->type != Token::RightPar) {
				delete expr;
				expected(begin, Token::RightPar);
			}
			return expr;
		} break;

		default:
		break;
	}
	expected(begin, Token::Identifier, Token::Integer, Token::LeftPar);
	return 0;
}

ASTExpression *parseExpr(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ASTExpression *lhs = parseSimpleExpr(begin, end);
	Token::Type a = begin->type;

	if(!isOperator(a)) {
		return lhs;
	}
	begin++;
	ASTExpression *mhs = parseSimpleExpr(begin, end);

	for(;;) {
		Token::Type b = begin->type;

		if(isOperator(b)) {
			begin++;
		} else {
			return new ASTBinOp(lhs, mhs, a);
		}

		ASTExpression *rhs = parseSimpleExpr(begin, end);

		if(assoc(b) > assoc(a)) {
			mhs = new ASTBinOp(mhs, rhs, b);
		} else {
			lhs = new ASTBinOp(lhs, mhs, a);
			mhs = rhs;
			a = b;
		}
	}
	return 0;
}

ASTDeclaration *parseDeclaration(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	eat(begin, Token::Var);

	expect(begin, Token::Identifier);
	core::String name = (begin++)->string;

	eat(begin, Token::Colon);

	expect(begin, Token::Identifier);
	core::String type = (begin++)->string;

	if(begin->type == Token::Assign) {
		begin++;
		return new ASTDeclaration(name, type, (begin - 1)->position, parseExpr(begin, end));
	}
	return new ASTDeclaration(name, type, (begin - 1)->position);
}


ASTInstruction *parseInstruction(core::Array<Token>::const_iterator &begin, core::Array<Token>::const_iterator end) {
	ASTInstruction *instr = 0;
	switch(begin->type) {
		case Token::Var:
			instr = parseDeclaration(begin, end);
		break;

		case Token::If: {
			begin++;
			expect(begin, Token::LeftPar);
			ASTExpression *expr = parseExpr(begin, end);
			return new ASTBranchInstruction(expr, parseInstruction(begin, end), 0);
		}	break;

		case Token::While: {
			begin++;
			expect(begin, Token::LeftPar);
			ASTExpression *expr = parseExpr(begin, end);
			return new ASTLoopInstruction(expr, parseInstruction(begin, end));
		} break;

		case Token::LeftBrace: {
			begin++;
			core::Array<ASTInstruction *> instrs;
			while(begin->type != Token::RightBrace) {
				instrs.append(parseInstruction(begin, end));
			}
			begin++;
			return new ASTInstructionList(instrs);
		} break;

		default:
			instr = new ASTExprInstruction(parseExpr(begin, end));
		break;
	}
	if(instr) {
		eat(begin, Token::SemiColon);
		return instr;
	}
	expected(begin, Token::Var, Token::While, Token::If, Token::LeftBrace);
	return 0;
}






Parser::Parser() {
}

ASTInstruction *Parser::parse(core::Array<Token>::const_iterator begin, core::Array<Token>::const_iterator end) const {
	core::Array<ASTInstruction *> instrs;
	while(begin != end && !(begin->type & Token::isEnd)) {
		instrs.append(parseInstruction(begin, end));
	}
	return instrs.size() == 1 ? instrs.first() : new ASTInstructionList(instrs);
}

}
}
