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
	BytecodeAssembler assembler;
	Context context{
		core::Map<WTFunction *, BytecodeAssembler>(),
		&assembler,
		ts,
		false};

	compile(context, node);

	assembler.ret(0);
	assembler.exit();

	for(const core::Pair<WTFunction * const, BytecodeAssembler> &p : context.externalAssemblers) {
		assembler << p._2;
	}

	return assembler;
}

void BytecodeCompiler::compile(Context &context, WTInstruction *node) {
	switch(node->type) {

		case WTNode::Block:
			for(WTInstruction *i : as<WTBlock>(node)->instructions) {
				compile(context, i);
			}
		return;


		case WTNode::Expression:
			compile(context, as<WTExprInstr>(node)->expression);
		return;


		case WTNode::Loop: {
			BytecodeAssembler::Label loop = context.assembler->createLabel();
			compile(context, as<WTLoop>(node)->condition);

			BytecodeAssembler::Label condJmp = context.assembler->createLabel();
			context.assembler->nope();

			compile(context, as<WTLoop>(node)->body);

			if(context.useIfDoWhile) {
				compile(context, as<WTLoop>(node)->condition);
				context.assembler->jumpNZ(as<WTLoop>(node)->condition->registerIndex, loop);
			} else {
				context.assembler->jump(loop);
			}

			context.assembler->seek(condJmp);
			context.assembler->jumpZ(as<WTLoop>(node)->condition->registerIndex, context.assembler->end());
			context.assembler->seek(context.assembler->end());
		}
		return;


		case WTNode::Branch: {
			compile(context, as<WTBranch>(node)->condition);

			BytecodeAssembler::Label condJmp = context.assembler->createLabel();
			context.assembler->nope();

			compile(context, as<WTBranch>(node)->thenBody);

			if(as<WTBranch>(node)->elseBody) {
				BytecodeAssembler::Label elseJmp = context.assembler->createLabel();
				context.assembler->nope();

				context.assembler->seek(condJmp);
				context.assembler->jumpZ(as<WTLoop>(node)->condition->registerIndex, context.assembler->end());
				context.assembler->seek(context.assembler->end());

				compile(context, as<WTBranch>(node)->elseBody);

				context.assembler->seek(elseJmp);
				context.assembler->jump(context.assembler->end());
			} else {
				context.assembler->seek(condJmp);
				context.assembler->jumpZ(as<WTLoop>(node)->condition->registerIndex, context.assembler->end());
			}
			context.assembler->seek(context.assembler->end());
		}
		return;


		case WTNode::Return:
			compile(context, as<WTReturn>(node)->value);
			context.assembler->ret(as<WTReturn>(node)->value->registerIndex);
		return;


		default:
			throw CompilationErrorException("Unknown node type", node);
	}
}

void BytecodeCompiler::compile(Context &context, WTFunction *func) {
	BytecodeAssembler *ass = context.assembler;
	context.assembler = &context.externalAssemblers[func];

	context.assembler->function(func->index, func->stackSize, func->args.size());
	compile(context, func->body);
	context.assembler->exit();

	context.assembler = ass;
}

void BytecodeCompiler::compile(Context &context, WTExpression *node) {
	switch(node->type) {

		case WTNode::Add:
		case WTNode::Substract:
		case WTNode::Multiply:
		case WTNode::Divide:
		case WTNode::Equals:
		case WTNode::NotEquals:
		case WTNode::LessThan:
		case WTNode::GreaterThan:
			compile(context, as<WTBinOp>(node));
		return;


		case WTNode::Assignation:
			compile(context, as<WTAssignation>(node)->value);
			if(node->registerIndex != as<WTAssignation>(node)->value->registerIndex) {
				context.assembler->copy(node->registerIndex, as<WTAssignation>(node)->value->registerIndex);
			}
		return;

		case WTNode::Call:
			if(!context.externalAssemblers.exists(as<WTCall>(node)->func)) {
				compile(context, as<WTCall>(node)->func);
			}
			for(WTExpression *e : as<WTCall>(node)->args) {
				compile(context, e);
				context.assembler->pushArg(e->registerIndex);
			}
			context.assembler->call(as<WTCall>(node)->registerIndex, as<WTCall>(node)->func->index);
		return;


		case WTNode::Integer:
			context.assembler->set(node->registerIndex, as<WTInt>(node)->value);
		return;


		case WTNode::Variable:
		return;


		default:
			throw CompilationErrorException("Unknown node type", node);
	}
}


void BytecodeCompiler::compile(Context &context, WTBinOp *node) {
	compile(context, node->lhs);
	compile(context, node->rhs);
	uint to = node->registerIndex;
	uint l = node->lhs->registerIndex;
	uint r = node->rhs->registerIndex;
	switch(node->type) {
		case WTNode::Add:
			context.assembler->addI(to, l, r);
		return;

		case WTNode::Substract:
			context.assembler->subI(to, l, r);
		return;

		case WTNode::Multiply:
			context.assembler->mulI(to, l, r);
		return;

		case WTNode::Divide:
			context.assembler->divI(to, l, r);
		return;

		case WTNode::Equals:
			context.assembler->equals(to, l, r);
		return;

		case WTNode::NotEquals:
			context.assembler->notEq(to, l, r);
		return;

		case WTNode::LessThan:
			context.assembler->lessI(to, l, r);
		return;

		case WTNode::GreaterThan:
			context.assembler->greaterI(to, l, r);
		return;

		default:
			throw CompilationErrorException("Unknown node type", node);
	}
}

}
}
