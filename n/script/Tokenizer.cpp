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
		if(tks.last().type & Token::isEnd) {
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
		return Token(Token::End, "\0", beg);
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
			return Token(Token::Var, tk, b);
		} else if(tk == "if") {
			return Token(Token::If, tk, b);
		} else if(tk == "else") {
			return Token(Token::Else, tk, b);
		} else if(tk == "while") {
			return Token(Token::While, tk, b);
		} else if(tk == "def") {
			return Token(Token::Def, tk, b);
		} else if(tk == "return") {
			return Token(Token::Return, tk, b);
		}
		return Token(Token::Identifier, tk, b);
	}
	if(isNum) {
		uint b = beg;
		beg = end;
		return Token(Token::Integer, tk, b);
	}
	if(tk.beginsWith("==")) {
		beg += 2;
		return Token(Token::Equals, "==", beg - 2);
	}
	if(tk.beginsWith("!=")) {
		beg += 2;
		return Token(Token::NotEquals, "!=", beg - 2);
	}
	if(tk.beginsWith("=")) {
		return Token(Token::Assign, "=", beg++);
	}
	if(tk.beginsWith("<")) {
		return Token(Token::LessThan, "<", beg++);
	}
	if(tk.beginsWith(">")) {
		return Token(Token::GreaterThan, ">", beg++);
	}
	if(tk.beginsWith("+")) {
		return Token(Token::Plus, "+", beg++);
	}
	if(tk.beginsWith("-")) {
		return Token(Token::Minus, "-", beg++);
	}
	if(tk.beginsWith("*")) {
		return Token(Token::Multiply, "*", beg++);
	}
	if(tk.beginsWith("/")) {
		return Token(Token::Divide, "/", beg++);
	}
	if(tk.beginsWith("(")) {
		return Token(Token::LeftPar, "(", beg++);
	}
	if(tk.beginsWith(")")) {
		return Token(Token::RightPar, ")", beg++);
	}
	if(tk.beginsWith("{")) {
		return Token(Token::LeftBrace, "{", beg++);
	}
	if(tk.beginsWith("}")) {
		return Token(Token::RightBrace, "}", beg++);
	}
	if(tk.beginsWith(",")) {
		return Token(Token::Coma, ",", beg++);
	}
	if(tk.beginsWith(":")) {
		return Token(Token::Colon, ":", beg++);
	}
	if(tk.beginsWith(";")) {
		return Token(Token::SemiColon, ";", beg++);
	}

	return Token(Token::Error, tk, beg = end);
}



}
}
