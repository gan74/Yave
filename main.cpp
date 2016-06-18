
#include <iostream>
#include <n/types.h>
#include <n/core/Collection.h>
#include <n/core/List.h>
#include <n/core/Map.h>

using namespace n;

template<typename T>
struct DEREF // P = false
{

	template<typename U>
	static TrueType test(decltype(&U::operator*) *) {
	std::cout << "lele"<<std::endl;
	}

	template<typename U>
	static FalseType test(...) {
		std::cout << "wat ?" << std::endl;
	}

	static constexpr bool value = decltype(test<T>(0))::value;
};

int main(int, char **) {
	using L = core::List<Nothing>;
	using C = core::Collection<L>;
	using I = typename L::iterator;
	static_assert(TypeInfo<L>::isIterable && TypeInfo<L>::isNonConstIterable, "lelle");
	std::cout << (typeid(typename core::Collection<L>::Element).name()) << std::endl;

	std::cout << TypeInfo<decltype(&I::operator*)>::type.name() << std::endl;

	 DEREF<I>::test<I>(0);
	std::cout << TypeInfo<typename TypeContent<I>::type>::type.name() << std::endl;
	return 0;
}
