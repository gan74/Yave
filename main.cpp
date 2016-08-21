
#include <iostream>
#include <vector>

#include <y/utils.h>
#include <y/core/Range.h>
#include <y/test/test.h>
#include <y/core/Vector.h>
#include <y/core/Ptr.h>

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



int main(int, char **) {
	range(0, 10).collect<Vector>();
	range(0, 10).collect(std::cout);


	return 0;
}
