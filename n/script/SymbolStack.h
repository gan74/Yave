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
#ifndef N_SCRIPT_SYMBOLSTACK_H
#define N_SCRIPT_SYMBOLSTACK_H

#include <n/core/String.h>
#include <n/core/Map.h>
#include "Token.h"

namespace n {
namespace script {

/*template<typename Symbol>
class NoErrorPolicy
{
	protected:
		void alreadyDeclared(const core::String &, const Symbol &) const {
		}

		Symbol &notDeclared(const core::String &) const {
			static Symbol nil;
			return nil;
		}
};

template<typename Symbol>
class ExceptionErrorPolicy
{
	protected:
		void alreadyDeclared(const core::String &name, const Symbol &, const Token &tk) const {
			throw ValidationErrorException("\"" + name + "\" already declared", tk);
		}

		Symbol &notDeclared(const core::String &name, const Token &tk) const {
			static Symbol nil;
			throw ValidationErrorException("\"" + name + "\" has not been declared", tk);
			return nil;
		}
};*/

template<typename Symbol>
class SymbolStack
{
	public:
		SymbolStack() {
			pushStack();
		}

		Symbol &get(const core::String &name) {
			typename SMap::iterator it = symbols.find(name);
			return it->_2;
		}

		Symbol &declare(const core::String &name, const Symbol &symbol) {
			typename SMap::iterator it = symbols.insert(name, symbol);
			stack.last().append(it);
			return it->_2;
		}

		bool isDeclared(const core::String &name) const {
			return symbols.exists(name);
		}

		void pushStack() {
			stack.append(core::Array<typename SMap::iterator>());
		}

		void popStack() {
			for(typename SMap::iterator it : stack.last()) {
				symbols.remove(it);
			}
			stack.pop();
		}

	private:
		using SMap = core::Map<core::String, Symbol>;

		SMap symbols;
		core::Array<core::Array<typename SMap::iterator>> stack;
};

}
}

#endif // N_SCRIPT_SYMBOLSTACK_H
