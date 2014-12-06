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

#ifndef N_SCRIPT_LEXER_H
#define N_SCRIPT_LEXER_H

#include <n/core/Functor.h>
#include "Token.h"

namespace n {
namespace script {


class Lexer
{
	public:
		struct Exception : public std::exception
		{
			Exception(const core::String &err) : msg(err) {
			}

			const char *what() const noexcept override {
				return msg.toChar();
			}

			const core::String msg;
		};

		class const_iterator
		{
			public:
				const_iterator(const const_iterator &it) : const_iterator(it.token, it.lexer) {
				}

				const_iterator &operator++() { // ++prefix
					token = lexer->next();
					return *this;
				}

				const_iterator operator++(int) { // postfix++
					const_iterator it(*this);
					operator++();
					return it;
				}

				bool operator==(const const_iterator &it) const {
					return lexer == it.lexer && token == it.token;
				}

				bool operator!=(const const_iterator &it) const {
					return !operator==(it);
				}

				const Token &operator*() const {
					return token;
				}

			protected:
				friend class Lexer;

				Lexer *lexer;
				Token token;

				const_iterator(const Token &t, Lexer *l) : lexer(l), token(t) {
				}
		};

		Lexer(const core::Functor<char> &n) : lineNum(1), colNum(0), nextChar(n) {
		}

		const_iterator begin() {
			return const_iterator(next(), this);
		}

		const_iterator end() {
			return const_iterator(Token(Token::End), this);
		}

	private:
		friend class const_iterator;

		Token next() {
			if(buffer.isEmpty()) {
				fillBuffer();
			}
			while(!buffer.isEmpty() && iswspace(buffer[0])) {
				buffer = buffer.subString(1);
			}
			if(buffer.isEmpty()) {
				return Token(Token::End);
			}
			core::Array<Token::Type> types = tokenTypes();
			for(uint i = 1; i != buffer.size(); i++) {
				core::String sub = buffer.subString(0, i);
				core::Array<Token::Type> tp = tokenTypes().filtered([=](Token::Type t) {
					return Token::matches(t, sub);
				});
				if(tp.isEmpty()) {
					types.sort();
					Token token(types.first(), lineNum, colNum, buffer.subString(0, i - 1));
					buffer = buffer.subString(i - 1);
					colNum += i;
					return token;
				}
				types = tp;
			}
			if(!buffer.isEmpty() && !types.isEmpty()) {
				types.sort();
				Token token(types.first(), lineNum, colNum, buffer);
				buffer = "";
				return token;
			}
			throw Exception("Unable to parse \"" + buffer + "\"");
		}

		void fillBuffer() {
			while(true) {
				char c = nextChar();
				if(c == '\0') {
					nextChar = []() { return 0; };
					break;
				}
				if(c == '\n') {
					lineNum++;
					colNum = 0;
					if(!buffer.isEmpty()) {
						break;
					}
				}
				buffer += c;
			}
		}

		core::Array<Token::Type> tokenTypes() const {
			return core::Array<Token::Type>(Token::Identifier, Token::Int, Token::Float, Token::String, Token::LeftPar, Token::RightPar, Token::LeftBrace, Token::RightBrace, Token::LeftBracket, Token::RightBracket, Token::Colon, Token::Comma, Token::SemiColon, Token::Dot, Token::Plus, Token::Minus, Token::Times, Token::Div, Token::Pow);
		}

		uint lineNum;
		uint colNum;
		core::String buffer;
		core::Functor<char> nextChar;
};

}
}

#endif // N_SCRIPT_LEXER_H
