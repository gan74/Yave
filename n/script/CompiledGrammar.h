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
#ifndef N_SCRIPT_COMPILEDGRAMMAR_H
#define N_SCRIPT_COMPILEDGRAMMAR_H

#include "Grammar.h"

namespace n {
namespace script {

class GrammarValidationException : public std::exception
{
	public:
		virtual const char *what() const noexcept override;
		virtual const char *what(const core::String &code) const noexcept;

	private:
		friend class CompiledGrammar;

		GrammarValidationException(const core::Array<TokenType> &e, core::Array<Token>::const_iterator p) : expected(e), position(p) {
		}

		core::Array<TokenType> expected;
		core::Array<Token>::const_iterator position;
};

class CompiledGrammar
{
	public:
		CompiledGrammar();

		core::Array<TokenType> getExpected() const;

		bool validate(const core::Array<Token> &tokens) const;

	//private:
		friend class Grammar;

		bool validate(core::Array<Token>::const_iterator begin, core::Array<Token>::const_iterator end, core::Array<Token>::const_iterator &last) const;

		core::Array<CompiledGrammar *> nexts[uint(TokenType::End) + 1];
};

}
}

#endif // N_SCRIPT_COMPILEDGRAMMAR_H
