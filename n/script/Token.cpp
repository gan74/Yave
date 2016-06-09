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

#include "Token.h"

namespace n {
namespace script {

uint TokenPosition::getLineNum(const core::String &code) const {
	if(!isValid()) {
		return 0;
	}
	uint line = 0;
	for(uint i = 0; i != index; i++) {
		line += code[i] == '\n';
	}
	return line;
}

core::String TokenPosition::getLine(const core::String &code) const {
	if(!isValid()) {
		return "";
	}
	for(uint i = 0;; i++) {
		if(code[index - i] == '\n') {
			uint beg = index - i + 1;
			return code.subString(beg, code.find("\n", index) - code.begin() - beg);
		}
	}
	return "";
}

uint TokenPosition::getColumn(const core::String &code) const {
	if(!isValid()) {
		return 0;
	}
	uint col = 0;
	for(uint i = index;; i--, col++) {
		if(code[i] == '\n') {
			return col - 1;
		}
		if(code[i] == '\t') {
			col += 3;
		}
	}
	return uint(-1);
}

core::String TokenPosition::toString(const core::String &code) const {
	core::String lineStr("at line ");
	lineStr += (getLineNum(code) + 1);
	lineStr += ": \"";

	core::String pointer;
	for(uint i = 0, end = lineStr.size() + getColumn(code); i != end; i++) {
		pointer += "~";
	}
	pointer += "^";
	return lineStr + getLine(code) + "\"\n" + pointer;
}

}
}
