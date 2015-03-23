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

#ifndef N_SCRIPT_TOKEN_H
#define N_SCRIPT_TOKEN_H

#include <n/core/String.h>
#include <n/defines.h>
#ifndef N_NO_SCRIPT

namespace n {
namespace script {

struct Token
{
	enum Type
	{
		End,
		Identifier,
		Int,
		Float,
		String,
		LeftPar,
		RightPar,
		LeftBrace,
		RightBrace,
		LeftBracket,
		RightBracket,
		Colon,
		Comma,
		SemiColon,
		Dot,
		Plus,
		Minus,
		Times,
		Div,
		Pow
	};


	Token(Type t, uint l = 0, uint c = 0, const core::String &str = "") : type(t), line(l), col(c), string(str) {
	}

	bool operator==(const Token &t) const {
		return type == t.type && (type == End || (line == t.line && col == t.col));
	}

	bool operator!=(const Token &t) const {
		return !operator==(t);
	}

	core::String toString() const {
		switch(type) {
			case Identifier:
			case Int:
			case Float:
			case String:
			case LeftPar:
			case RightPar:
			case LeftBrace:
			case RightBrace:
			case LeftBracket:
			case RightBracket:
			case Colon:
			case Comma:
			case SemiColon:
			case Dot:
			case Plus:
			case Minus:
			case Times:
			case Div:
			case Pow:
				return string;
			default:
				fatal("Unknown token type");
		}
		return "ERROR";
	}

	static bool matches(Type t, core::String str) {
		switch(t) {
			case Identifier:
				return str.size() &&
					   !isdigit(str[0]) &&
					   AsCollection(str).forall([](char c) { return isalnum(c) || c == '_'; });
			case Int:
				return AsCollection(str).forall([](char c) { return isdigit(c); });
			case Float: {
				uint p = 0;
				return AsCollection(str).forall([&p](char c) -> bool {
					if(c == '.') {
						p++;
						return true;
					}
					return isdigit(c);
				}) && p == 1;
			}
			case String:
				return str.size() > 1 &&
					   str[0] == '\"' &&
					   str[str.size() - 1] == '\"';
			case LeftPar:
				return str == "(";
			case RightPar:
				return str == ")";
			case LeftBrace:
				return str == "{";
			case RightBrace:
				return str == "}";
			case LeftBracket:
				return str == "[";
			case RightBracket:
				return str == "]";
			case Colon:
				return str == ":";
			case Comma:
				return str == ",";
			case SemiColon:
				return str == ";";
			case Dot:
				return str == ".";
			case Plus:
				return str == "+";
			case Minus:
				return str == "-";
			case Times:
				return str == "*";
			case Div:
				return str == "/";
			case Pow:
				return str == "^";
			default:
				return fatal("Unknown token type");
		}
	}


	Type type;
	uint line;
	uint col;
	core::String string;
};

}
}

#endif

#endif // N_SCRIPT_TOKEN_H
