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
#include "WTBuilder.h"

namespace n {
namespace script {

const char *ValidationErrorException::what() const noexcept {
	return msg.data();
}

const char *ValidationErrorException::what(const core::String &code) const noexcept {
	buffer = msg + "\n" + position.toString(code);
	return buffer.data();
}


WTBuilder::WTBuilder() : types(new WTTypeSystem()) {
	enterFunction();
}

WTBuilder::~WTBuilder() {
}

WTBuilder::FuncData &WTBuilder::getLocalData() {
	return funcStack.last();
}

const WTBuilder::FuncData &WTBuilder::getLocalData() const {
	return funcStack.last();
}

WTTypeSystem *WTBuilder::getTypeSystem() const {
	return types;
}

WTFunction *WTBuilder::getCurrentFunction() const {
	return funcStack.last().func;
}

void WTBuilder::enterScope() {
	FuncData &func = getLocalData();
	VarStackData vdat;
	vdat.index = func.varStack.last().index;
	func.varStack.append(vdat);
}

void WTBuilder::leaveScope() {
	FuncData &func = getLocalData();
	for(typename VMap::iterator it : func.varStack.last().all) {
		func.vars.remove(it);
	}
	func.maxReg = std::max(func.maxReg, func.varStack.last().index);
	func.varStack.pop();
}

void WTBuilder::enterFunction(WTFunction *function) {
	funcStack.append(FuncData());
	enterScope();

	FuncData &data = funcStack.last();

	uint index = 0;
	if(function) {
		for(WTVariable *v : function->args) {
			index = std::max(index, v->registerIndex);
			data.vars[v->name] = v;
		}
	}

	data.func = function;
	data.maxReg = index;
	data.varStack.last().index = index;
}

void WTBuilder::leaveFunction() {
	leaveScope();

	FuncData &data = funcStack.last();
	if(data.func) {
		data.func->stackSize = data.maxReg;
	}

	funcStack.pop();
}

uint WTBuilder::allocRegister() {
	return getLocalData().varStack.last().index++;
}

WTVariable *WTBuilder::declareVar(const core::String &name, const core::String &typeName, TokenPosition tk) {
	FuncData &func = getLocalData();
	if(func.vars.exists(name)) {
		throw ValidationErrorException("\"" + name + "\" has already been declared in this scope", tk);
	}
	WTVariableType *type = types->getType(typeName);
	if(!type) {
		throw ValidationErrorException("\"" + typeName + "\" is not a type", tk);
	}
	WTVariable *v = new WTVariable(name, type, allocRegister());

	typename VMap::iterator it = func.vars.insert(name, v);
	func.varStack.last().all.append(it);

	return v;
}

WTVariable *WTBuilder::getVar(const core::String &name, TokenPosition tk) const {
	const FuncData &func = getLocalData();
	typename VMap::const_iterator it = func.vars.find(name);
	if(it == func.vars.end()) {
		throw ValidationErrorException("\"" + name + "\" has not been declared in this scope", tk);
	}
	return it->_2;
}

WTFunction *WTBuilder::declareFunc(const core::String &name, const core::Array<WTVariable *> &args, WTVariableType *ret, TokenPosition tk) {
	if(funcMap.exists(name)) {
		throw ValidationErrorException("\"" + name + "\" has already been declared in this scope", tk);
	}
	WTFunction *f = new WTFunction(name, args, 0, ret, funcMap.size());

	funcMap.insert(name, f);

	return f;
}

WTFunction *WTBuilder::getFunc(const core::String &name, TokenPosition tk) const {
	typename FMap::const_iterator it = funcMap.find(name);
	if(it == funcMap.end()) {
		throw ValidationErrorException("\"" + name + "\" has not been declared in this scope", tk);
	}
	return it->_2;
}

}
}
