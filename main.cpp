#include <n/core/Array.h>
#include <n/core/String.h>
#include <n/Types.h>
#include <n/core/Set.h>
#include <n/core/Timer.h>
#include <n/test/Test.h>

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

Array<int> testArray;

template<typename T>
uint64 testComp(uint max) {
	T set;
	uint64 eq = 0;
	uint64 comp = 0;
	for(uint i = 0; i != max; i++) {
		set.insert(ComparisonCounter(&eq, &comp));
	}
	return eq + comp;
}

template<typename T>
void testSet() {
	T s;
	for(int i : testArray) {
		s.insert(i);
		if(i % 7 == 0) {
			s.insert(i);
		}
	}
	return;





	int i = 0;
	for(int e : s) {
		if(e != i) {
			nError("!");
		}
		i++;
	}
	int max = testArray.size();
	for(int w = 0; w < max; w += w % 3 ? 5 : 13) {
		auto it = s.find(w);
		if(it == s.end() || *it != w) {
			nError("!");
		}
	}
}

void randInit(int max) {
	testArray.clear();
	testArray.setCapacity(max);
	for(int i = 0; i != max; i++) {
		testArray.append(i);
	}
	testArray.shuffle();
}

int main(int, char **) {
	uint max = 100000; //3250
	randInit(max);
	Timer timer;
	testSet<Set<int>>();
	double nset = timer.reset() * 1000;
	testSet<std::set<int>>();
	double stdset = timer.reset() * 1000;
	std::cout<<max<<" insertions :"<<std::endl<<"nset   = "<<nset<<"ms"<<std::endl<<"stdset = "<<stdset<<"ms"<<std::endl;

	return 0;
}



