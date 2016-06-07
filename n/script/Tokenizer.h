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
#ifndef N_SCRIPT_TOKENIZER_H
#define N_SCRIPT_TOKENIZER_H

#include <n/core/String.h>
#include <n/core/Array.h>

namespace n {
namespace script {

enum class TokenType
{
	Identifier,
	Integer,
	Float,

	Assign,
	Equals,

	Plus,
	Minus,
	Multiply,
	Divide,

	LeftPar,
	RightPar,

	Colon,
	SemiColon,

	Var,

	Error,
	End
};

static constexpr const char *tokenName[] = {"identifier", "integer", "float", "=", "==", "+", "-", "*", "/", "'('", "')'", "':'", "';'", "'var'", "", "EOF"};

class Token
{
	public:
		Token(TokenType t, const core::String s = "", uint i = 0) : type(t), string(s), index(i) {
		}

		bool isEnd() const {
			return type == TokenType::End || type == TokenType::Error;
		}

		const TokenType type;
		const core::String string;
		const uint index;
};

class Tokenizer
{
	public:
		Tokenizer();

		core::Array<Token> tokenize(const core::String &code);

	private:
		Token next(const core::String &str, uint &beg);
};

}
}

#endif // N_SCRIPT_TOKENIZER_H
