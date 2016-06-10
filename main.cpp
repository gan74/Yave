#include <n/core/String.h>
#include <n/script/Parser.h>
#include <n/core/Map.h>
#include <n/core/Timer.h>
#include <iostream>
#include <n/script/WTNode.h>
#include <n/script/WTBuilder.h>
#include <n/script/BytecodeCompiler.h>
#include <n/script/Machine.h>

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
	core::String code = "var x:Int = 44444;					\n"
						"var y:Int = 55555;					\n"
						"y = 66666;							\n"
						"x = y;								\n";


	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;

	try {
		ASTInstruction *node = parser.parse(tks);
		std::cout << node->toString() << std::endl << std::endl;

		WTBuilder builder;
		WTInstruction *wt = node->toWorkTree(builder);

		BytecodeCompiler compiler;
		BytecodeAssembler ass = compiler.compile(wt, builder.getTypeSystem());
		ass.exit();


		for(BytecodeInstruction i : ass.getInstructions()) {
			std::cout << i.op << " $" << i.registers[0] << " ";
			if(i.op == Bytecode::Set) {
				std::cout << i.data() << std::endl;
			} else {
				std::cout << "$" << i.registers[1] << " $" << i.registers[2] << std::endl;
			}
		}

		Machine machine;
		int *memory = machine.run(ass.getInstructions().begin());

		std::cout << "--------------------------------------------------------------------------------" << std::endl;
		for(uint i = 0; i != 8; i++) {
			std::cout << std::hex << i << " ";
			std::cout << memory[i] << std::endl;
		}
		delete[] memory;


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
