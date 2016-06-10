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
#ifndef N_SCRIPT_WTVARIABLETYPE_H
#define N_SCRIPT_WTVARIABLETYPE_H

#include <n/core/String.h>

namespace n {
namespace script {

class WTVariableType : NonCopyable
{
	public:
		const core::String &getName() const;
		bool isObject() const;

	private:
		friend class WTVariablePrimitiveType;
		friend class WTVariableObjectType;

		WTVariableType(const core::String &typeName, bool obj);

		const core::String name;
		const bool object;
};

class WTVariablePrimitiveType : public WTVariableType
{
	public:
		WTVariablePrimitiveType(const core::String &name, int coerceOrder = 0) : WTVariableType(name, false), order(coerceOrder) {
		}

		int getCoercionOrder() const {
			return order;
		}

	private:
		int order;
};

class WTVariableObjectType : public WTVariableType
{
	public:
		WTVariableObjectType(const core::String &name) : WTVariableType(name, true) {
		}
};

class WTVariableIntType : public WTVariablePrimitiveType
{
	public:
		WTVariableIntType() : WTVariablePrimitiveType("Int", 0) {
		}
};

class WTVariableFloatType : public WTVariablePrimitiveType
{
	public:
		WTVariableFloatType() : WTVariablePrimitiveType("Float", 1) {
		}
};

}
}

#endif // N_SCRIPT_WTVARIABLETYPE_H
