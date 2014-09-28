
#include <n/core/Array.h>
#include <n/core/String.h>
#include <n/Types.h>
#include <n/core/Set.h>
#include <n/core/Timer.h>
#include <n/test/Test.h>
#include <n/core/Map.h>
#include <n/core/Functor.h>
#include <n/core/SmartPtr.h>

#include <iostream>
#include <set>


using namespace n::core;
using namespace n;

struct ComparisonCounter
{
	ComparisonCounter(uint64 *e, uint64 *i) : eq(e), inf(i), id(rand()) {
	}

	bool operator==(const ComparisonCounter &cc) const {
		(*eq)++;
		return id == cc.id;
	}

	bool operator<(const ComparisonCounter &cc) const {
		(*inf)++;
		return id < cc.id;
	}

	uint64 *eq;
	uint64 *inf;
	int id;
};

struct IntFunc
{
	int operator ()(int i) const {
		return 3 * i;
	}
};


int main(int, char **) {
	std::cout<<std::boolalpha;
	return 0;
}



