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
#ifndef N_SCRIPT_EXECUTIONVAR_H
#define N_SCRIPT_EXECUTIONVAR_H

#include <n/types.h>

namespace n {
namespace script {
namespace ast {

class ExecutionType;

struct ExecutionVar
{
	const ExecutionType *type;
	union
	{
		int64 integer;
		double real;
		void *object;
	};

	ExecutionVar() : type(0), integer(0) {
	}

	ExecutionVar(const ExecutionType *t, int64 i) : type(t), integer(i) {
	}

	ExecutionVar(const ExecutionType *t, double d) : type(t), real(d) {
	}

	ExecutionVar(const ExecutionType *t, void *p) : type(t), object(p) {
	}

	//ExecutionVar(ExecutionType *t);
};

}
}
}

#endif // N_SCRIPT_EXECUTIONVAR_H
