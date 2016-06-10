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
#include "WTTypeSystem.h"

namespace n {
namespace script {

WTTypeSystem::WTTypeSystem() {
	addType(new WTVariableIntType());
	addType(new WTVariableFloatType());
}

void WTTypeSystem::addType(WTVariableType *type) {
	types[type->getName()] = type;
}

WTVariableType *WTTypeSystem::getType(const core::String &name) const {
	core::Map<core::String, WTVariableType *>::const_iterator it = types.find(name);
	return it == types.end() ? 0 : it->_2;
}

WTVariableType *WTTypeSystem::getIntType() const {
	return getType("Int");
}

WTVariableType *WTTypeSystem::getFloatType() const {
	return getType("Float");
}

bool WTTypeSystem::assign(WTVariableType *lhs, WTVariableType *rhs) {
	if(!lhs->isObject() && !rhs->isObject()) {
		return primitiveOpCoerce(lhs, rhs) == lhs;
	}
	return false;
}

WTVariableType *WTTypeSystem::add(WTVariableType *lhs, WTVariableType *rhs) {
	if(!lhs->isObject() && !rhs->isObject()) {
		return primitiveOpCoerce(lhs, rhs);
	}
	return 0;
}

WTVariableType *WTTypeSystem::primitiveOpCoerce(WTVariableType *lhs, WTVariableType *rhs) {
	WTVariablePrimitiveType *l = static_cast<WTVariablePrimitiveType *>(lhs);
	WTVariablePrimitiveType *r = static_cast<WTVariablePrimitiveType *>(rhs);
	return l->getCoercionOrder() < r->getCoercionOrder() ? r : l;
}


}
}
