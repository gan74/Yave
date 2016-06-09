#include <n/core/String.h>
#include <n/script/Parser.h>
#include <n/core/Map.h>
#include <n/core/Timer.h>
#include <iostream>
#include <n/script/WorkTreeNode.h>
#include <n/script/WorkTreeBuilder.h>

using namespace n;
using namespace n::core;
using namespace n::script;


/*template<typename T>
const T *as(const ast::Node *n) {
	return reinterpret_cast<const T *>(n);
}

ast::ExecutionVar evalE(const ast::Expression *node, ast::ExecutionFrame &frame) {
	uint index = node->index;
	switch(node->type) {
		case ast::NodeType::Identifier:
			return frame.varStack.getVar(as<ast::Identifier>(node)->name, index);

		case ast::NodeType::Assignation: {
			ast::ExecutionVar &e = frame.varStack.getVar(as<ast::Assignation>(node)->name);
			return e = evalE(as<ast::Assignation>(node)->value, frame);
		}

		case ast::NodeType::Add: {
			ast::ExecutionVar l = evalE(as<ast::BinOp>(node)->lhs, frame);
			ast::ExecutionVar r = evalE(as<ast::BinOp>(node)->rhs, frame);
			return l.type->add(l, r);
		}

		case ast::NodeType::Substract: {
			ast::ExecutionVar l = evalE(as<ast::BinOp>(node)->lhs, frame);
			ast::ExecutionVar r = evalE(as<ast::BinOp>(node)->rhs, frame);
			return l.type->sub(l, r);
		}

		case ast::NodeType::Multiply: {
			ast::ExecutionVar l = evalE(as<ast::BinOp>(node)->lhs, frame);
			ast::ExecutionVar r = evalE(as<ast::BinOp>(node)->rhs, frame);
			return l.type->mul(l, r);
		}

		case ast::NodeType::Divide: {
			ast::ExecutionVar l = evalE(as<ast::BinOp>(node)->lhs, frame);
			ast::ExecutionVar r = evalE(as<ast::BinOp>(node)->rhs, frame);
			return l.type->div(l, r);
		}

		case ast::NodeType::Equals: {
			ast::ExecutionVar l = evalE(as<ast::BinOp>(node)->lhs, frame);
			ast::ExecutionVar r = evalE(as<ast::BinOp>(node)->rhs, frame);
			return frame.intType->init(l.type->equals(l, r));
		}

		case ast::NodeType::NotEquals: {
			ast::ExecutionVar l = evalE(as<ast::BinOp>(node)->lhs, frame);
			ast::ExecutionVar r = evalE(as<ast::BinOp>(node)->rhs, frame);
			return frame.intType->init(!l.type->equals(l, r));
		}

		case ast::NodeType::Integer:
			return frame.intType->init(as<ast::Integer>(node)->value);

		default:
			fatal("ERROR");
	}
	return fatal("ERR");
}

void evalI(const ast::Instruction *node, ast::ExecutionFrame &frame) {
	uint index = node->index;
	switch (node->type) {
		case ast::NodeType::Expression: {
			ast::ExecutionVar r = evalE(as<ast::ExprInstruction>(node)->expression, frame);
			if(frame.print && as<ast::ExprInstruction>(node)->print) {
				r.type->print(r);
			}
		} break;

		case ast::NodeType::Branch: {
			ast::ExecutionVar c = evalE(as<ast::BranchInstruction>(node)->condition, frame);
			if(c.integer) {
				evalI(as<ast::BranchInstruction>(node)->thenBody, frame);
			} else if(as<ast::BranchInstruction>(node)->elseBody) {
				evalI(as<ast::BranchInstruction>(node)->elseBody, frame);
			}
		} break;

		case ast::NodeType::Loop: {
			ast::ExecutionVar c = evalE(as<ast::LoopInstruction>(node)->condition, frame);
			while(c.integer) {
				evalI(as<ast::LoopInstruction>(node)->body, frame);
				c = evalE(as<ast::LoopInstruction>(node)->condition, frame);
			}
		} break;

		case ast::NodeType::InstructionList: {
			frame.varStack.pushStack();
			for(ast::Instruction *i : as<ast::InstructionList>(node)->instructions) {
				evalI(i, frame);
			}
			frame.varStack.popStack();
		} break;

		case ast::NodeType::Declaration: {
			if(frame.varStack.isDeclared(as<ast::Declaration>(node)->name)) {
				throw ast::ExecutionException("\"" + as<ast::Declaration>(node)->name + "\" has already been declared", index);
			}
			core::Map<core::String, ast::ExecutionType *>::iterator it = frame.types.find(as<ast::Declaration>(node)->typeName);
			if(it == frame.types.end()) {
				throw ast::ExecutionException("\"" + as<ast::Declaration>(node)->typeName + "\" is not a type", index);
			}
			ast::ExecutionVar &a = frame.varStack.declare(as<ast::Declaration>(node)->name, it->_2);
			if(as<ast::Declaration>(node)->value) {
				ast::ExecutionVar v = evalE(as<ast::Declaration>(node)->value, frame);
				if(a.type != v.type) {
					throw ast::ExecutionException("Incompatible type affected to \"" + as<ast::Declaration>(node)->name + "\"", index);
				}
				a = v;
			}
		} break;

		default:
			fatal("ERROR");
	}
}*/


int main(int, char **) {
	core::String code = "var x:Int = 400000;					\n"
						"var y:Int = 1;							\n"
						"var z:Float = 1000000;					\n"
						"if(x == y) var i:Int;"
						"while(x - 2 != 2 * 3) {				\n"
						"	x = x - 1;							\n"
						"	y = y + 1;							\n"
						"	z = z / 2;							\n"
						"	i = i + 1;							\n"
						"	if(y == 4643) { y; }				\n"
						"}										\n"
						"y;	z;									\n";


	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;

	try {
		ASTInstruction *node = parser.parse(tks);
		std::cout << node->toString() << std::endl << std::endl;

		WorkTreeBuilder builder;
		node->toWorkTree(builder);



		/*{
			std::cout << std::endl;
			Timer timer;
			node->eval(frame);
			std::cout << "eval = " << timer.elapsed() * 1000 << "ms" << std::endl;
		}*/

	} catch(SynthaxErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	} catch(ValidationErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	}
	return 0;
}
