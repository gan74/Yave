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

void print(uint index, BytecodeInstruction i) {
	std::cout << index<< "\t";
	Map<Bytecode, core::String> names;
	names[Bytecode::AddI] = "addi";
	names[Bytecode::SubI] = "subi";
	names[Bytecode::MulI] = "muli";
	names[Bytecode::DivI] = "divi";
	names[Bytecode::LessI] = "lessi";
	names[Bytecode::GreaterI] = "gri";
	names[Bytecode::Equals] = "eq";
	names[Bytecode::NotEq] = "neq";
	switch(i.op) {
		case Bytecode::Set:
			std::cout << "set $" << i.registers[0] << " " << i.data();
		break;

		case Bytecode::Copy:
			std::cout << "cpy $" << i.registers[0] << " $" << i.registers[1];
		break;

		case Bytecode::Not:
			std::cout << "not $" << i.registers[0] << " $" << i.registers[1];
		break;

		case Bytecode::Jump:
			std::cout << "jmp " << i.data() + 1;
		break;

		case Bytecode::JumpZ:
			std::cout << "jmpz $" << i.registers[0] << " " << i.data() + 1;
		break;

		case Bytecode::JumpNZ:
			std::cout << "jmpnz $" << i.registers[0] << " " << i.data() + 1;
		break;

		case Bytecode::Call:
			std::cout << "call $" << i.registers[0] << " " << i.data() + 1;
		break;

		case Bytecode::PushArg:
			std::cout << "push $" << i.registers[0];
		break;

		case Bytecode::FuncHead:
			std::cout << "function";
		break;

		case Bytecode::Ret:
			std::cout << "ret $" << i.registers[0];
		break;

		case Bytecode::Exit:
			std::cout << "exit";
		break;

		default:
			std::cout << names[i.op] << " $" << i.registers[0] << " $" << i.registers[1] << " $" << i.registers[2];


	}
	std::cout << std::endl;
}


int main(int, char **) {
	core::String code = "var x:Int = 1000100;					\n"
						"var y:Int = x - x;						\n"
						"if(x > 1000) {							\n"
						"	y = 1000;							\n"
						"} else {								\n"
						"	y = 7;								\n"
						"}										\n"
						"var a:Int = 13;						\n"
						"x = foobar(x, y);						\n"
						"def foobar(var a:Int, var y:Int) = {	\n"
						"	while(a != y) {						\n"
						"		a = a - 1;						\n"
						"	}									\n"
						"	return 21;							\n"
						"}										\n";
						//"var a:Int = 9;							\n"
						//"var w:Int = foobar(x);					\n";


	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;
	WTBuilder builder;

	try {
		ASTInstruction *node = parser.parse(tks);
		std::cout << node->toString() << std::endl << std::endl;

		node->resolveFunctions(builder);
		WTInstruction *wt = node->toWorkTree(builder);

		BytecodeCompiler compiler;
		BytecodeAssembler ass = compiler.compile(wt, builder.getTypeSystem());


		uint index = 0;
		for(BytecodeInstruction i : ass.getInstructions()) {
			if(i.op == Bytecode::FuncHead) {
				index = 0;
			}
			print(index++, i);
		}

		Timer timer;
		Machine machine;
		machine.load(ass.getInstructions().begin(), ass.getInstructions().end());
		Machine::Primitive ret = machine.run(ass.getInstructions().begin());
		std::cout << "return " << ret << std::endl << "eval = " << timer.elapsed() * 1000 << "ms" << std::endl;
	} catch(SynthaxErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	} catch(ValidationErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	}

	return 0;
}
