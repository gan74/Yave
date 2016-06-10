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
#include "BytecodeCompiler.h"
#include <iostream>

namespace n {
namespace script {

template<typename T, typename U>
T *as(U *n) {
	return static_cast<T *>(n);
}


BytecodeCompiler::BytecodeCompiler() {
}

BytecodeAssembler BytecodeCompiler::compile(WTInstruction *node, WTTypeSystem *ts) {
	Context context;
	context.typeSystem = ts;

	compile(context, node);

	return context.assembler;
}

void BytecodeCompiler::compile(Context &context, WTInstruction *node) {
	switch(node->type) {
		case WTNode::Block:
			for(WTInstruction *i : as<WTBlock>(node)->instructions) {
				compile(context, i);
			}
		break;

		case WTNode::Expression:
			compile(context, as<WTExprInstr>(node)->expression);
		break;



		default:
			throw CompilationErrorException("Unknown node type", node);
	}
}

void BytecodeCompiler::compile(Context &context, WTExpression *node) {
	switch(node->type) {
		case WTNode::Add:
			compile(context, as<WTBinOp>(node)->lhs);
			compile(context, as<WTBinOp>(node)->rhs);
			context.assembler.addI(node->registerIndex, as<WTBinOp>(node)->lhs->registerIndex, as<WTBinOp>(node)->rhs->registerIndex);
		break;

		case WTNode::Multiply:
			compile(context, as<WTBinOp>(node)->lhs);
			compile(context, as<WTBinOp>(node)->rhs);
			context.assembler.mulI(node->registerIndex, as<WTBinOp>(node)->lhs->registerIndex, as<WTBinOp>(node)->rhs->registerIndex);
		break;

		case WTNode::Assignation:
			compile(context, as<WTAssignation>(node)->value);
			if(node->registerIndex != as<WTAssignation>(node)->value->registerIndex) {
				context.assembler.copy(node->registerIndex, as<WTAssignation>(node)->value->registerIndex);
			}
		break;

		case WTNode::Integer:
			context.assembler.set(node->registerIndex, as<WTInt>(node)->value);
		break;

		case WTNode::Variable:
		break;

		default:
			throw CompilationErrorException("Unknown node type", node);
	}
}

}
}
