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
#include "ExecutionType.h"
#include "ExecutionException.h"
#include <iostream>

namespace n {
namespace script {
namespace ast {

ExecutionType::ExecutionType(const core::String &typeName) : name(typeName) {
}

void ExecutionType::expectType(ExecutionVar a, const ExecutionType *type, uint index) const {
	if(a.type != type) {
		throw ExecutionException("Invalid type \"" + a.type->name + "\" expected \"" + type->name + "\"", index);
	}
}

bool ExecutionType::equals(ExecutionVar a, ExecutionVar b, uint index) const {
	expectType(a, b.type, index);
	return a.integer == b.integer;
}





bool ExecutionIntType::lessThan(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return a.integer < b.integer;
}

bool ExecutionIntType::greaterThan(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return a.integer > b.integer;
}

ExecutionVar ExecutionIntType::add(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.integer + b.integer);
}

ExecutionVar ExecutionIntType::sub(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.integer - b.integer);
}

ExecutionVar ExecutionIntType::mul(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.integer * b.integer);
}

ExecutionVar ExecutionIntType::div(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.integer / b.integer);
}

void ExecutionIntType::print(ExecutionVar a) const {
	//expectType(a, this);
	std::cout << a.integer << std::endl;
}





bool ExecutionFloatType::lessThan(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return a.real < b.real;
}

bool ExecutionFloatType::greaterThan(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return a.real > b.real;
}

ExecutionVar ExecutionFloatType::add(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.real + b.real);
}

ExecutionVar ExecutionFloatType::sub(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.real - b.real);
}

ExecutionVar ExecutionFloatType::mul(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.real * b.real);
}

ExecutionVar ExecutionFloatType::div(ExecutionVar a, ExecutionVar b, uint index) const {
	//expectType(a, this, index);
	expectType(b, this, index);
	return ExecutionVar(this, a.real / b.real);
}

void ExecutionFloatType::print(ExecutionVar a) const {
	//expectType(a, this);
	std::cout << a.real << std::endl;
}

}
}
}
