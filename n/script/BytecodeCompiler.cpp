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
	context.useIfDoWhile = true;
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


		case WTNode::Loop: {
			BytecodeAssembler::Label loop = context.assembler.createLabel();
			compile(context, as<WTLoop>(node)->condition);

			BytecodeAssembler::Label condJmp = context.assembler.createLabel();
			context.assembler.nope();

			compile(context, as<WTLoop>(node)->body);

			if(context.useIfDoWhile) {
				compile(context, as<WTLoop>(node)->condition);
				context.assembler.jumpNZ(as<WTLoop>(node)->condition->registerIndex, loop);
			} else {
				context.assembler.jump(loop);
			}

			BytecodeAssembler::Label end = context.assembler.createLabel();
			context.assembler.seek(condJmp);
			context.assembler.jumpZ(as<WTLoop>(node)->condition->registerIndex, end);

			context.assembler.seek(context.assembler.end());
		} break;

		default:
			throw CompilationErrorException("Unknown node type", node);
	}
}

void BytecodeCompiler::compile(Context &context, WTExpression *node) {
	switch(node->type) {

		case WTNode::Add:
		case WTNode::Substract:
		case WTNode::Multiply:
		case WTNode::Divide:
		case WTNode::NotEquals: {
			compile(context, as<WTBinOp>(node)->lhs);
			compile(context, as<WTBinOp>(node)->rhs);
			uint to = node->registerIndex;
			uint l = as<WTBinOp>(node)->lhs->registerIndex;
			uint r = as<WTBinOp>(node)->rhs->registerIndex;
			switch(node->type) {
				case WTNode::Add:
					context.assembler.addI(to, l, r);
					return;

				case WTNode::Substract:
					context.assembler.subI(to, l, r);
					return;

				case WTNode::Multiply:
					context.assembler.mulI(to, l, r);
					return;

				case WTNode::Divide:
					context.assembler.divI(to, l, r);
					return;

				case WTNode::NotEquals:
					context.assembler.notEq(to, l, r);
					return;

				default:
					return;
			}
		}


		case WTNode::Assignation:
			compile(context, as<WTAssignation>(node)->value);
			if(node->registerIndex != as<WTAssignation>(node)->value->registerIndex) {
				context.assembler.copy(node->registerIndex, as<WTAssignation>(node)->value->registerIndex);
			}
			return;


		case WTNode::Integer:
			context.assembler.set(node->registerIndex, as<WTInt>(node)->value);
			return;


		case WTNode::Variable:
			return;


		default:
			throw CompilationErrorException("Unknown node type", node);
	}
}

}
}
