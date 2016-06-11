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
#ifndef N_SCRIPT_BYTECODECOMPILER_H
#define N_SCRIPT_BYTECODECOMPILER_H

#include "BytecodeAssembler.h"
#include "WTNode.h"
#include "WTTypeSystem.h"

namespace n {
namespace script {

class CompilationErrorException : public std::exception
{
	public:
		CompilationErrorException(const core::String &m, WTNode *n) : msg(m), node(n) {
		}

		virtual const char *what() const noexcept override {
			return msg.data();
		}

		WTNode *getNode() const {
			return node;
		}

	private:
		core::String msg;
		WTNode *node;
};

class BytecodeCompiler : NonCopyable
{
	struct Context
	{
		BytecodeAssembler assembler;
		WTTypeSystem *typeSystem;

		bool useIfDoWhile;
	};

	public:
		BytecodeCompiler();

		BytecodeAssembler compile(WTInstruction *node, WTTypeSystem *ts);

	private:
		void compile(Context &context, WTInstruction *node);
		void compile(Context &context, WTExpression *node);

		void compile(Context &context, WTBinOp *node);
};

}
}

#endif // N_SCRIPT_BYTECODECOMPILER_H
