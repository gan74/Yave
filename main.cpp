#include <n/core/String.h>
#include <iostream>
#include <n/script/CompiledGrammar.h>

using namespace n;
using namespace n::core;
using namespace n::script;


void all(CompiledGrammar *g, Set<CompiledGrammar *> &s) {
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
	std::cout << g << ":" << std::endl;
	for(uint i = 0; i != 6; i++) {
		if(!g->nexts[i].isEmpty()) {
			std::cout << "  " << tokenName[i] << ":" << std::endl;
			for(auto x : g->nexts[i]) {
				std::cout << "    " << x << std::endl;
			}
		}
	}
	std::cout << std::endl;
}


int main(int, char **) {

	core::String code = "a+c-/b";

	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);
	tks.pop();

	/*for(auto t : tks) {
		std::cout << uint(t.type) << " '" << t.string << "'" << std::endl;
	}*/

	Grammar *expr = (Grammar *)malloc(sizeof(Grammar));
	Grammar p = Grammar::expect(expr, Plus, expr);
	Grammar m = Grammar::expect(expr, Minus, expr);
	new(expr) Grammar(Grammar::any(&m, &p, Identifier));
	/*Grammar grammar = Grammar::expect(expr, End);

	Set<CompiledGrammar *> grammars;
	all(expr->compile(), grammars);

	for(auto x : grammars) {
		print(x);
	}*/



	try {
		std::cout << std::boolalpha << (expr->compile()->validate(tks)) << std::endl;
	} catch(GrammarValidationException e) {
		std::cerr << e.what(code) << std::endl;
	}



	return 0;
}
