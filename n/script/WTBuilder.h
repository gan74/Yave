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
#ifndef N_SCRIPT_WTBUILDER_H
#define N_SCRIPT_WTBUILDER_H

#include "WTVariableStack.h"
#include "WTTypeSystem.h"
#include "WTFunction.h"

namespace n {
namespace script {

class ValidationErrorException : public std::exception
{
	public:
		ValidationErrorException(const core::String &m, TokenPosition tk) : msg(m), position(tk) {
		}

		virtual const char *what() const noexcept override;
		virtual const char *what(const core::String &code) const noexcept;

	private:
		core::String msg;
		TokenPosition position;

		mutable core::String buffer;
};


class WTBuilder : NonCopyable
{

	using VMap = core::Map<core::String, WTVariable *>;
	using FMap = core::Map<core::String, WTFunction *>;

	template<typename T>
	struct StackData
	{
		core::Array<typename core::Map<core::String, T>::iterator> all;
		uint index;
	};

	public:
		WTBuilder();
		~WTBuilder();

		WTVariable *declareVar(const core::String &name, const core::String &typeName, TokenPosition tk = TokenPosition());
		WTVariable *getVar(const core::String &name, TokenPosition tk = TokenPosition()) const;

		WTFunction *declareFunc(const core::String &name, const core::Array<WTVariable *> &args, WTInstruction *body, WTVariableType *ret, TokenPosition tk = TokenPosition());
		WTFunction *getFunc(const core::String &name, TokenPosition tk) const;

		void pushStack();
		void popStack();

		uint allocRegister();

		WTTypeSystem *getTypeSystem() const;

	private:
		uint allocSlot();

		WTTypeSystem *types;

		VMap varMap;
		core::Array<StackData<WTVariable *>> varStack;
		core::Array<WTVariable *> variables;

		FMap funcMap;
		core::Array<StackData<WTFunction *>> funcStack;
		core::Array<WTFunction *> functions;


};

}
}

#endif // WORKTREEBUILDER_H
