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
#ifndef N_SCRIPT_EXECUTIONTYPE_H
#define N_SCRIPT_EXECUTIONTYPE_H

#include <n/core/String.h>
#include "ExecutionVar.h"

namespace n {
namespace script {
namespace ast {

class ExecutionType
{
	public:
		ExecutionType(const core::String &typeName);

		const core::String name;

		virtual ExecutionVar init(int64 value = 0) {
			ExecutionVar v;
			v.type = this;
			v.integer = value;
			return v;
		}

		virtual bool lessThan(ExecutionVar a, ExecutionVar b, uint index = uint(-1)) const = 0;
		virtual bool greaterThan(ExecutionVar a, ExecutionVar b, uint index = uint(-1)) const = 0;
		virtual bool equals(ExecutionVar a, ExecutionVar b, uint index = uint(-1)) const = 0;

		virtual ExecutionVar add(ExecutionVar a, ExecutionVar b, uint index = uint(-1)) const = 0;
		virtual ExecutionVar sub(ExecutionVar a, ExecutionVar b, uint index = uint(-1)) const = 0;
		virtual ExecutionVar mul(ExecutionVar a, ExecutionVar b, uint index = uint(-1)) const = 0;
		virtual ExecutionVar div(ExecutionVar a, ExecutionVar b, uint index = uint(-1)) const = 0;

		virtual void print(ExecutionVar a) const = 0;

	protected:
		void expectType(ExecutionVar a, const ExecutionType *type, uint index = uint(-1)) const;

};

class ExecutionIntType : public ExecutionType
{
	public:
		ExecutionIntType() : ExecutionType("Int") {
		}

		virtual bool lessThan(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual bool greaterThan(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual bool equals(ExecutionVar a, ExecutionVar b, uint index) const override;

		virtual ExecutionVar add(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual ExecutionVar sub(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual ExecutionVar mul(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual ExecutionVar div(ExecutionVar a, ExecutionVar b, uint index) const override;

		virtual void print(ExecutionVar a) const override;
};

class ExecutionFloatType : public ExecutionType
{
	public:
		ExecutionFloatType() : ExecutionType("Float") {
		}

		virtual bool lessThan(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual bool greaterThan(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual bool equals(ExecutionVar a, ExecutionVar b, uint index) const override;

		virtual ExecutionVar add(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual ExecutionVar sub(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual ExecutionVar mul(ExecutionVar a, ExecutionVar b, uint index) const override;
		virtual ExecutionVar div(ExecutionVar a, ExecutionVar b, uint index) const override;

		virtual void print(ExecutionVar a) const override;
};

}
}
}

#endif // N_SCRIPT_EXECUTIONTYPE_H
