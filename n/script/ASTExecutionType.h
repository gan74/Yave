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
#ifndef N_SCRIPT_ASTEXECUTIONTYPE_H
#define N_SCRIPT_ASTEXECUTIONTYPE_H

#include <n/core/String.h>
#include "ASTExecutionVar.h"

namespace n {
namespace script {

class ASTExecutionType
{
	public:
		ASTExecutionType(const core::String &typeName);

		const core::String name;

		virtual bool lessThan(ASTExecutionVar a, ASTExecutionVar b) const = 0;
		virtual bool greaterThan(ASTExecutionVar a, ASTExecutionVar b) const = 0;
		virtual bool equals(ASTExecutionVar a, ASTExecutionVar b) const = 0;
		virtual void print(ASTExecutionVar a) const = 0;

		virtual ASTExecutionVar init(int64 value = 0) {
			ASTExecutionVar v;
			v.type = this;
			v.integer = value;
			return v;
		}

};

class ASTExecutionIntType : public ASTExecutionType
{
	public:
		ASTExecutionIntType() : ASTExecutionType("Int") {
		}

		virtual bool lessThan(ASTExecutionVar a, ASTExecutionVar b) const override {
			return a.integer < b.integer;
		}

		virtual bool greaterThan(ASTExecutionVar a, ASTExecutionVar b) const override {
			return a.integer > b.integer;
		}

		virtual bool equals(ASTExecutionVar a, ASTExecutionVar b) const override {
			return a.integer == b.integer;
		}

		virtual void print(ASTExecutionVar a) const override;
};

class ASTExecutionFloatType : public ASTExecutionType
{
	public:
		ASTExecutionFloatType() : ASTExecutionType("Float") {
		}

		virtual bool lessThan(ASTExecutionVar a, ASTExecutionVar b) const override {
			return a.real < b.real;
		}

		virtual bool greaterThan(ASTExecutionVar a, ASTExecutionVar b) const override {
			return a.real > b.real;
		}

		virtual bool equals(ASTExecutionVar a, ASTExecutionVar b) const override {
			return a.real == b.real;
		}

		virtual void print(ASTExecutionVar a) const override;
};


}
}

#endif // N_SCRIPT_ASTEXECUTIONTYPE_H
