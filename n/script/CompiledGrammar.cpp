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
#include "CompiledGrammar.h"

namespace n {
namespace script {

const char *GrammarValidationException::what() const noexcept {
	core::String str = "{ ";
	for(TokenType t : expected) {
		str << tokenName[t] << " ";
	}
	return ("Expected " + str + "} got " + tokenName[position->type] + " (\"" + position->string + "\")").data();
}

const char *GrammarValidationException::what(const core::String &code) const noexcept {
	core::String str = "{ ";
	for(TokenType t : expected) {
		str << tokenName[t] << " ";
	}

	uint line = 0;
	for(auto it = code.begin(); it < code.begin() + position->index; it = code.find('\n', it)) {
		line++;
	}
	core::String lineStr("at line ");
	lineStr << core::String2(line) << ": \"";

	str = "Expected " + str + "} got " + tokenName[position->type] + ":\n" + lineStr;

	uint lineBeg = position->index;
	for(; lineBeg != 0 && code[lineBeg - 1] != '\n'; lineBeg--);
	uint end = code.find('\n', lineBeg) - code.begin();

	str += code.subString(lineBeg, end - lineBeg) + "\"\n";
	for(uint i = lineBeg; i != position->index + lineStr.size(); i++) {
		str += " ";
	}
	str += "^";

	return str.data();
}

CompiledGrammar::CompiledGrammar() {
}

core::Array<TokenType> CompiledGrammar::getExpected() const {
	core::Array<TokenType> tks;
	for(uint i = 0; i != uint(TokenType::End) + 1; i++) {
		if(!nexts[i].isEmpty()) {
			tks += TokenType(i);
		}
	}
	return tks;
}

bool CompiledGrammar::validate(const core::Array<Token> &tokens) const {
	auto last = tokens.begin();
	return validate(tokens.begin(), tokens.end(), last);
}

bool CompiledGrammar::validate(core::Array<Token>::const_iterator begin, core::Array<Token>::const_iterator end, core::Array<Token>::const_iterator &last) const {
	struct Elem
	{
		core::Array<Token>::const_iterator it;
		const CompiledGrammar *grammar;
		uint index;

	};

	core::Array<Elem> stack{Elem{begin, this, 0}};
	for(;;) {
		Elem current = stack.last();
		if(current.it == end) {
			return true;
		}
		if(current.grammar->nexts[current.it->type].size() <= current.index) {
			last = std::max(current.it, last);
			stack.pop();
			if(stack.isEmpty()) {
				throw GrammarValidationException(current.grammar->getExpected(), last);
			}
			stack.last().index++;
		} else {
			stack += Elem{current.it + 1, current.grammar->nexts[current.it->type][current.index], 0};
		}
	}
	return false;

}

}
}
