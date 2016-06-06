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
		return Token(End);
	}

	bool alnum = !!isalnum(str[beg]);
	uint end = beg + 1;
	while(end != str.size()) {
		if(isspace(str[end]) || !!isalnum(str[end]) != alnum) {
			break;
		} else {
			end++;
		}
	}

	core::String tk = str.subString(beg, end - beg);


	if(alnum) {
		uint b = beg;
		beg = end;
		return Token(Identifier, tk, b);
	}
	if(tk.beginsWith("==")) {
		beg += 2;
		return Token(Equals, "==", beg - 2);
	}
	if(tk.beginsWith("=")) {
		return Token(Assign, "=", beg++);
	}
	if(tk.beginsWith("+")) {
		return Token(Plus, "+", beg++);
	}
	if(tk.beginsWith("-")) {
		return Token(Minus, "-", beg++);
	}
	if(tk.beginsWith("*")) {
		return Token(Multiply, "*", beg++);
	}
	if(tk.beginsWith("/")) {
		return Token(Divide, "/", beg++);
	}




	return Token(Error, tk, beg = end);
}



}
}
