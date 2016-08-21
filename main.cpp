
#include <iostream>
#include <vector>
#include <y/utils.h>
#include <y/core/Range.h>
#include <y/test/test.h>
#include <y/core/Vector.h>

using namespace y;
using namespace core;

template<typename T>
void print(T t) {
	std::cout << t << " ";
}


template<typename T>
Vector<T> &operator<<(Vector<T> &vec, const T &value) {
	vec.append(value);
	return vec;
}

template<typename T>
using Vec = Vector<T>;


int main(int, char **) {


	range(0, 10).collect<Vec>();
	range(0, 10).collect(std::cout);


	return 0;
}
