
#include <iostream>
#include <vector>
#include <y/core/Range.h>

using namespace y;
using namespace core;

template<typename T>
void print(T t) {
	std::cout << t << " ";
}

/*void text_const(const std::vector<int> &c) {
	(*range(c).begin()) = 7;
	(*range(c).reverse().begin()) = 7;
}

void text_non_const(std::vector<int> c) {
	*(range(c).reverse().begin()) = 7;
	*(range(c).begin()) = 8;
}*/

int main(int, char **) {
	std::cout << "0..10" << std::endl;
	range(0, 10).foreach(print<int>);
	std::cout << std::endl << std::endl;

	std::cout << "10..0" << std::endl;
	range(10, 0).foreach(print<int>);
	std::cout << std::endl << std::endl;

	std::cout << "0..10 % 2" << std::endl;
	range(0, 10).map([](int i) { return i % 2; }).foreach(print<int>);
	std::cout << std::endl << std::endl;


	return 0;
}
