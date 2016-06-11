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

namespace n {
namespace script {


class TokenPosition
{
	public:
		uint getLineNum(const core::String &code) const;
		core::String getLine(const core::String &code) const;
		uint getColumn(const core::String &code) const;

		core::String toString(const core::String &code) const;

		bool isValid() const {
			return index != uint(-1);
		}

		TokenPosition() : index(-1) {
		}

	private:
		friend class Tokenizer;

		TokenPosition(uint i) : index(i) {
		}

		const uint index;
};

class Token
{
	public:
		static constexpr uint isOperator		= 0x08000000;
		static constexpr uint rightAssoc		= 0x04000000;
		static constexpr uint leftAssoc			= 0x02000000;
		static constexpr uint isEnd				= 0x10000000;

		static constexpr uint associativityMask	= rightAssoc | leftAssoc;
		static constexpr uint flagMask			= 0xFF000000;

		enum Type
		{
			Plus			= isOperator | leftAssoc | 0,
			Minus			= isOperator | leftAssoc | 1,
			Multiply		= isOperator | rightAssoc | 2,
			Divide			= isOperator | rightAssoc | 3,

			Equals			= isOperator | 4,
			NotEquals		= isOperator | 5,

			LessThan		= isOperator | 6,
			GreaterThan		= isOperator | 7,


			Assign			= 10,

			Identifier		= 11,
			Integer			= 12,
			Float			= 13,

			LeftPar			= 14,
			RightPar		= 15,

			LeftBrace		= 16,
			RightBrace		= 17,

			Colon			= 18,
			SemiColon		= 19,

			Var				= 20,
			If				= 21,
			Else			= 22,
			While			= 23,

			Error			= isEnd | 24,
			End				= isEnd | 25
		};

		static core::String getName(Type type) {
			static constexpr const char *names[] = {"'+'", "'-'", "'*'", "'/'", "'=='", "'!='", "'<'", "'>'", "", "",
													"'='", "identifier", "integer", "float", "'('", "')'", "'{'", "'}'", "':'", "';'",
													"'var'", "'if'", "'else'","'while'", "", "EOF"};
			return names[type & ~flagMask];
		}

		Token(Type t = Error, const core::String s = "", TokenPosition p = TokenPosition()) : type(t), string(s), position(p) {
		}

		const Type type;
		const core::String string;
		const TokenPosition position;
};

}
}

#endif // N_SCRIPT_TOKEN_H
