#include <n/core/String.h>
#include <n/script/Parser.h>
#include <n/core/Map.h>
#include <n/core/Timer.h>
#include <n/script/ast/ExecutionFrame.h>
#include <n/script/ast/ExecutionException.h>
#include <iostream>

using namespace n;
using namespace n::core;
using namespace n::script;

int main(int, char **) {
	core::String code = "var x:Int = 400000;					\n"
						"var y:Int = 1;							\n"
						"while(x) {								\n"
						"	x = x - 1;							\n"
						"	y = y + 1;							\n"
						"	if(y == 46435) { y; }				\n"
						"}										\n"
						"y;										\n";


	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;


	ast::ExecutionFrame frame;
	//frame.print = false;
	frame.addType(new ast::ExecutionFloatType());

	try {
		ast::Instruction *node = parser.parse(tks);
		std::cout << node->toString() << std::endl;

		Timer timer;
		node->eval(frame);
		std::cout << "eval = " << timer.elapsed() * 1000 << "ms" << std::endl;
	} catch(SynthaxErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	} catch(ast::ExecutionException &e) {
		std::cerr << e.what(code) << std::endl;
	}


	/*Tokenizer tokenizer;
	Parser parser;

	ast::ExecutionFrame frame;

	char linec[256];
	for(;;) {
		std::cin.getline(linec, 256);
		String line((const char *)linec);
		auto tks = tokenizer.tokenize(line);
		try {
			ast::Instruction *node = parser.parse(tks.begin(), tks.end());
			node->eval(frame);
			delete node;
		}catch(SynthaxErrorException &e) {
			std::cerr << e.what() << std::endl;
		} catch(ast::ExecutionException &e) {
			std::cerr << e.what() << std::endl;
		}
	}*/

	return 0;
}
