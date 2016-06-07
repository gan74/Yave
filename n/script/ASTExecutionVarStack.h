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
#ifndef N_SCRIPT_ASTEXECUTIONVARSTACK_H
#define N_SCRIPT_ASTEXECUTIONVARSTACK_H

#include <n/core/Map.h>
#include <n/core/Array.h>
#include <n/core/String.h>
#include "ASTExecutionType.h"

namespace n {
namespace script {

class ASTExecutionVarStack
{
	public:
		ASTExecutionVarStack();

		ASTExecutionVar &declare(const core::String &name, ASTExecutionType *type);

		void pushStack();
		void popStack();

		ASTExecutionVar &getVar(const core::String &name);
		bool isDeclared(const core::String &name) const;


	private:
		using VarMap = core::Map<core::String, ASTExecutionVar>;

		ASTExecutionVar invalid;
		VarMap vars;
		core::Array<core::Array<VarMap::iterator>> stack;
};

}
}
#endif // N_SCRIPT_ASTEXECUTIONVARSTACK_H
