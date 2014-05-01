#include <n/core/Array.h>
#include <n/core/Pair.h>
#include <n/core/RPointer.h>
#include <n/core/Lazy.h>
#include <n/core/Memo.h>
#include <n/core/Option.h>
#include <n/core/String.h>
#include <n/io/File.h>
#include <n/test/TestTemplate.h>
#include <n/Types.h>
#include <n/test/Test.h>
#include <n/core/BigUint.h>

#include <iostream>


int main(int, char **) {
	std::cout<<n::TypeInfo<int>::id<<std::endl;
	std::cout<<n::TypeInfo<int *>::id<<std::endl;
	std::cout<<n::TypeInfo<const int *>::id<<std::endl;
	std::cout<<std::endl;
	std::cout<<n::TypeInfo<int *>::baseId<<std::endl;
	std::cout<<n::TypeInfo<const int *>::baseId<<std::endl;
	std::cout<<n::TypeInfo<const int>::baseId<<std::endl;

	return 0;
}



