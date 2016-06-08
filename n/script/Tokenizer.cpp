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
#include "Tokenizer.h"
#include <iostream>

namespace n {
namespace script {

Tokenizer::Tokenizer() {
}

core::Array<Token> Tokenizer::tokenize(const core::String &code) {
	core::Array<Token> tks;
	for(uint i = 0;;) {
		tks << next(code, i);
		if(tks.last().isEnd()) {
			break;
		}
	}
	return tks;
}

Token Tokenizer::next(const core::String &str, uint &beg) {
	while(beg != str.size()) {
		if(isspace(str[beg])) {
			beg++;
		} else {
			break;
		}
	}

	if(beg == str.size()) {
		return Token(TokenType::End, "\0", beg);
	}

	bool isNum = !!isdigit(str[beg]);
	bool isAlpha = !!isalpha(str[beg]);
	//bool alnum = isNum || isAlpha;
	uint end = beg + 1;
	while(end != str.size()) {
		if(isspace(str[end]) || (isAlpha && !isalnum(str[end])) || (isNum && !isdigit(str[end]))) {
			break;
		} else {
			end++;
		}
	}

	core::String tk = str.subString(beg, end - beg);


	if(isAlpha) {
		uint b = beg;
		beg = end;
		if(tk == "var") {
			return Token(TokenType::Var, tk, b);
		} else if(tk == "if") {
			return Token(TokenType::If, tk, b);
		} else if(tk == "while") {
			return Token(TokenType::While, tk, b);
		}
		return Token(TokenType::Identifier, tk, b);
	}
	if(isNum) {
		uint b = beg;
		beg = end;
		return Token(TokenType::Integer, tk, b);
	}
	if(tk.beginsWith("==")) {
		beg += 2;
		return Token(TokenType::Equals, "==", beg - 2);
	}
	if(tk.beginsWith("!=")) {
		beg += 2;
		return Token(TokenType::NotEquals, "!=", beg - 2);
	}
	if(tk.beginsWith("=")) {
		return Token(TokenType::Assign, "=", beg++);
	}
	if(tk.beginsWith("+")) {
		return Token(TokenType::Plus, "+", beg++);
	}
	if(tk.beginsWith("-")) {
		return Token(TokenType::Minus, "-", beg++);
	}
	if(tk.beginsWith("*")) {
		return Token(TokenType::Multiply, "*", beg++);
	}
	if(tk.beginsWith("/")) {
		return Token(TokenType::Divide, "/", beg++);
	}
	if(tk.beginsWith("(")) {
		return Token(TokenType::LeftPar, "(", beg++);
	}
	if(tk.beginsWith(")")) {
		return Token(TokenType::RightPar, ")", beg++);
	}
	if(tk.beginsWith(":")) {
		return Token(TokenType::Colon, ":", beg++);
	}
	if(tk.beginsWith(";")) {
		return Token(TokenType::SemiColon, ";", beg++);
	}

	return Token(TokenType::Error, tk, beg = end);
}



}
}
