#include <n/core/String.h>
#include <iostream>
//#include <n/script/CompiledGrammar.h>
#include <n/script/Parser.h>

using namespace n;
using namespace n::core;
using namespace n::script;

/*void all(CompiledGrammar *g, Set<CompiledGrammar *> &s) {
	if(s.find(g) == s.end()) {
		s += g;
		for(uint i = 0; i != uint(TokenType::End) + 1; i++) {
			for(CompiledGrammar *c : g->nexts[i]) {
				all(c, s);
			}
		}
	}
}

void print(CompiledGrammar *g) {
	std::cout << g->name << ":" ;
	if(g->terminal) {
		std::cout << "(term)";
	}
	std::cout << std::endl;

	for(uint i = 0; i != 6; i++) {
		if(!g->nexts[i].isEmpty()) {
			std::cout << "  " << tokenName[i] << ":" << std::endl;
			for(auto x : g->nexts[i]) {
				std::cout << "    " << x->name << std::endl;
			}
		}
	}
	std::cout << std::endl;
}*/


int main(int, char **) {

	core::String code = "var a:Int = 7; x = a + b; lol; b = 7;";

	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;


	try {
		ASTNode *node = parser.parse(tks.begin(), tks.end());
		std::cout << std::boolalpha << node->toString() << std::endl;
	} catch(SynthaxErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	}

	/*Grammar *expr = new Grammar();
	Grammar id = Grammar::expect("id", Identifier);
	Grammar p = Grammar::expect("+", expr, Plus, expr);
	Grammar m = Grammar::expect("-", expr, Minus, expr);
	*expr = Grammar::any("expr", &id, &m, &p);
	//Grammar grammar = Grammar::expect("g", expr, End);


	Set<CompiledGrammar *> cc;
	all(expr->compile(), cc);
	for(auto g : cc) {
		print(g);
	}

	try {
		std::cout << std::boolalpha << (expr->compile()->validate(tks)) << std::endl;
	} catch(GrammarValidationException e) {
		std::cerr << e.what(code) << std::endl;
	}*/



	return 0;
}
