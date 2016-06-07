#include <n/core/String.h>
#include <n/script/Parser.h>
#include <n/core/Map.h>
#include <iostream>

using namespace n;
using namespace n::core;
using namespace n::script;

struct Test {};

/*template<typename T>
struct TypeHasIterator {

	struct NoType { };
	struct Fallback { NoType iterator; };

	template<typename B>
	struct Derived : B, Fallback { };

	template<typename V>
	static FalseType test(decltype(Derived<V>::iterator) *);

	template<typename V>
	static BoolToType<!std::is_same<typename V::iterator *, V *>::value> test(typename V::iterator *);

	static constexpr bool value = decltype(test<T>(0))::value;
};*/

N_GEN_TYPE_HAS_MEMBER2(TypeHasIterator, iterator)


int main(int, char **) {
	std::cout << std::boolalpha << bool(TypeHasIterator<int>::value) << std::endl;
	std::cout<< "lele"<<std::endl;
	std::cout << std::boolalpha << TypeHasIterator<Map<int, int>::iterator>::value << std::endl;
	std::cout << std::boolalpha << TypeHasIterator<Map<int, int>>::value << std::endl;
	std::cout << std::boolalpha << TypeHasIterator<Test>::value << std::endl << std::endl;

	//std::cout << Type(decltype(Map<int, int>::iterator::iterator)).name() << std::endl;

	std::cout << std::boolalpha << TypeInfo<Map<int, int>::const_iterator>::isIterable << std::endl;
	std::cout << std::boolalpha << TypeInfo<Map<int, int>::iterator>::isIterable << std::endl;
	std::cout << std::boolalpha << TypeInfo<Array<int>::iterator>::isIterable << std::endl << std::endl;

	std::cout << std::boolalpha << TypeInfo<Map<int, int>>::isIterable << std::endl;
	std::cout << std::boolalpha << TypeInfo<Array<int>>::isIterable << std::endl;

	core::String code = "var a:Int = 7; x = a + 1 * b * c; lol; b = 7;";

	Tokenizer tokenizer;
	auto tks = tokenizer.tokenize(code);

	Parser parser;


	try {
		ASTNode *node = parser.parse(tks.begin(), tks.end());
		std::cout << std::boolalpha << node->toString() << std::endl;
	} catch(SynthaxErrorException &e) {
		std::cerr << e.what(code) << std::endl;
	}

	return 0;
}
