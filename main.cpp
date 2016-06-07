#include <n/core/String.h>
#include <n/script/Parser.h>
#include <n/core/Map.h>
#include <n/script/ASTExecutionFrame.h>
#include <iostream>

using namespace n;
using namespace n::core;
using namespace n::script;


int main(int, char **) {
	core::String code = "var x:Int = 7; x = x + 2;";

	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;


	try {
		ASTInstruction *node = parser.parse(tks.begin(), tks.end());
		std::cout << std::boolalpha << node->toString() << std::endl;

		ASTExecutionFrame frame;
		node->eval(frame);
	} catch(SynthaxErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	} catch(ASTExecutionException &e) {
		std::cerr << e.what(code) << std::endl;
	}

	return 0;
}
