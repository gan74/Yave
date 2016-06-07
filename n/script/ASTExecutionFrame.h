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
#ifndef N_SCRIPT_ASTEXECUTIONFRAME_H
#define N_SCRIPT_ASTEXECUTIONFRAME_H

#include "ASTExecutionVarStack.h"

namespace n {
namespace script {

class ASTExecutionException : std::exception
{
	public:
		virtual const char *what() const noexcept override;
		virtual const char *what(const core::String &code) const noexcept;

		ASTExecutionException(const core::String &m, uint ind);

	private:
		core::String msg;
		uint index;

};

class ASTExecutionFrame
{
	public:
		ASTExecutionFrame();

		ASTExecutionVarStack varStack;

		core::Map<core::String, ASTExecutionType *> types;

		ASTExecutionIntType *intType;
};

}
}

#endif // N_SCRIPT_ASTEXECUTIONFRAME_H
