
#include <y/core/Chrono.h>
#include <y/core/Lazy.h>
#include <y/math/math.h>

#include <unordered_map>
#include <iostream>
#include <iomanip>

using namespace y;
using namespace y::core;

int create() {
	std::cout << "create()" << std::endl;
	return 7;
}

int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif

	Lazy<int> i(create);

	std::cout << "lazy = " << i() << std::endl;

	std::cout << 14 << std::endl;


	std::cout << std::setprecision(25) << math::pi<float> << '\n' << math::pi<double> << std::endl;
	return 0;
}




