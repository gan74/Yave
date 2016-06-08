#include <n/core/String.h>
#include <n/script/Parser.h>
#include <n/core/Map.h>
#include <n/script/ast/ExecutionFrame.h>
#include <n/script/ast/ExecutionException.h>
#include <iostream>

using namespace n;
using namespace n::core;
using namespace n::script;


int main(int, char **) {
	core::String code = "var x:Int = 7; x = (x + 2) * 3; 9 + 2;";

	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;


	try {
		ast::Instruction *node = parser.parse(tks.begin(), tks.end());
		std::cout << std::boolalpha << node->toString() << std::endl;

		ast::ExecutionFrame frame;
		node->eval(frame);
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
