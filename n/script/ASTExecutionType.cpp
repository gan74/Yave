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
#include "ASTExecutionType.h"
#include <iostream>

namespace n {
namespace script {

ASTExecutionType::ASTExecutionType(const core::String &typeName) : name(typeName) {
}

void ASTExecutionIntType::print(ASTExecutionVar a) const {
	std::cout << a.integer << std::endl;
}

void ASTExecutionFloatType::print(ASTExecutionVar a) const {
	std::cout << a.real << std::endl;
}

}
}
