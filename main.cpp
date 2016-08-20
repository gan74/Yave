
#include <iostream>
#include <vector>
#include <y/utils.h>
#include <y/core/Range.h>
#include <y/test/test.h>

using namespace y;
using namespace core;

template<typename T>
void print(T t) {
	std::cout << t << " ";
}

test_func("works test") {
	return true;
}

test_func("works test") {
	return true;
}

test_func("works test") {
	return true;
}

test_func("works test") {
	return true;
}

test_func("works test") {
	return true;
}


test_func("fails test") {
	return false;
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
