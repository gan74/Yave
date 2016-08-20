
#include <iostream>
#include <vector>
#include <y/utils.h>
#include <y/core/Range.h>

using namespace y;
using namespace core;
using namespace wildcard;

template<typename T>
void print(T t) {
	std::cout << t << " ";
}

void test_range() {
	std::cout << "0..10" << std::endl;
	range(0, 10).foreach(print<int>);
	std::cout << std::endl << std::endl;

	std::cout << "10..0" << std::endl;
	range(10, 0).foreach(print<int>);
	std::cout << std::endl << std::endl;

	std::cout << "0..10 % 2" << std::endl;
	range(0, 10).map([](int i) { return i % 2; }).foreach(print<int>);
	std::cout << std::endl << std::endl;
}

template<typename T>
Vec<T> &operator<<(Vec<T> &vec, const T &value) {
	vec.push_back(value);
	return vec;
}


int main(int, char **) {

	range(0, 10).collect<Vec>();
	range(0, 10).collect(std::cout);


	return 0;
}
