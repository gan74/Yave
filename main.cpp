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

#include <n/mem/Allocator.h>

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

	mem::Allocator<4, 60> alloc;
	Array<void *> ptrs;
	uint max = 4096;
	for(uint i = 0; i != max; i++) {

		void *ptr = 0;
		ptr = alloc.allocate();
		ptrs.append(ptr);
		if(i % 255 == 0 && i) {
			uint si  = random(ptrs.size() / 2, ptrs.size() / 16);
			std::cout<<"freeing "<<si<<"/"<<ptrs.size()<<" : "<<std::endl;
			for(uint w = 0; w != si; w++) {
				uint in = random(ptrs.size());
				void *p = ptrs[in];
				ptrs[in] = ptrs.last();
				ptrs.pop();
				alloc.desallocate(p);
			}
			std::cout<<"\t"<<ptrs.size()<<" live ptrs : "<<alloc.getChunkCount()<<" chunks ("<<alloc.getChunkCount() * 60<<" blocks)"<<std::endl;
			std::cout<<"\toptimal = "<<ptrs.size() / 60 + 1<<std::endl<<std::endl;
		}
	}
	std::cout<<"freeing memory...."<<std::endl;
	for(void *p : ptrs) {
		alloc.desallocate(p);
	}

	return 0;
}



