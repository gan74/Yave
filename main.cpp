#include <iostream>

#include<n/types.h>
#include <n/utils.h>
#include <n/core/Array.h>
#include <n/core/String.h>
#include <n/core/Set.h>
#include <n/core/Timer.h>
#include <n/test/Test.h>
#include <n/core/Map.h>
#include <n/core/Functor.h>
#include <n/core/SmartPtr.h>
#include <n/core/Lazy.h>

#include <n/mem/Allocator.h>

#include <n/concurent/Thread.h>
#include <n/concurent/Mutex.h>
#include <n/concurent/Promise.h>
#include <n/concurent/Async.h>
#include <n/concurent/ThreadPool.h>

#include <n/io/ConsoleStream.h>

#include <n/math/Vec.h>
#include <n/math/Matrix.h>
#include <n/math/Quaternion.h>
#include <n/math/Transform.h>

#include <n/assets/AssetBuffer.h>

#include <n/script/Lexer.h>
#include <n/script/Machine.h>

using namespace	n::concurent;
using namespace n::assets;
using namespace n::core;
using namespace n::math;
using namespace n::io;
using namespace n;

template<typename... Args>
String substr(const String &str, Args... args) {
	return str.subString(args...);
}

template<typename... Args>
std::string substr(const std::string &str, Args... args) {
	return str.substr(args...);
}

template<typename T>
T benchReplace(double &time) {
	Timer timer;
	T a("strReplaceBench");
	uint max = 300000;
	const char *test = "maybe";
	const char *test2 = "ornpot";
	for(uint i = 0; i != max; i++) {
		a.replace(3, 7, T(test));
		a.replace(7, 4, T(test2));
	}
	time += timer.elapsed();
	return a;
}

template<typename T>
uint benchCtor(double &time) {
	Timer timer;
	uint max = 500000;
	uint tot = 0;
	while(tot < max) {
		T a("maybe");
		T b("ornpot");
		tot += a.size() + b.size();
	}
	time += timer.elapsed();
	return tot;
}

template<typename T>
uint benchEq(double &time) {
	Timer timer;
	uint max = 40000000;
	T a("strReplacebench");
	uint tot = 0;
	while(tot < max) {
		T b("maybe");
		a = b;
		tot += a.size() + b.size();
	}
	time += timer.elapsed();
	return tot;
}

template<typename T>
T benchCat(double &time) {
	Timer timer;
	T a("strbench");
	uint max = 30000000;
	T tmp("Strings !");
	while(a.size() < max) {
		a = a + tmp;
		tmp += a;
	}
	while(a.size() > 453) {
		a =	substr(a, a.size() / 2, a.size() / 2 - 1);
	}
	time += timer.elapsed();
	return a;
}

template<typename T>
T benchMulti(double &time) {
	Timer timer;
	T a("strbench");
	uint max = 100000;
	T tmp("Strings !");
	for(uint i = 0; i != max; i++) {
		a = a + tmp;
		uint mod = 1025 + sin(i / 200.0) * 1000;
		if(a.size() > mod) {
			do {
				a = substr(a, mod / 7, a.size() % mod);
			} while(a.size() > 3 * mod);
		} else {
			a = substr(a, 1);
		}
		tmp = a;
		a = a + "mark";
	}
	time += timer.elapsed();
	return a;
}

void printScore(double &co, double &st) {
	std::cout<<"std::string  : "<<round(st * 1000)<<"ms"<<std::endl;
	std::cout<<"core::String : "<<round(co * 1000)<<"ms"<<std::endl;
	std::cout<<"Total        : "<<round((st + co) * 1000)<<"ms"<<std::endl;
	if(co > st) {
		std::cout<<"/!\\ core::String failed to outperform std::string"<<std::endl;
	} else {
		std::cout<<std::endl;
	}
	double min = std::min(co, st);
	std::cout<<"difference  : "<<round((st + co - min - min) / min * 1000) / 10<<"%"<<std::endl;
	std::cout<<std::endl;
	std::cout<<std::endl;
	co = st = 0;
}

void benchmark(double &stdTime, double &strTime, double time = 5) {
	stdTime = 0;
	strTime = 0;
	do {
		if(benchReplace<String>(strTime) != benchReplace<std::string>(stdTime)) {
			fatal("Strings not equal !");
		}//printScore(strTime, stdTime);
		if(benchCtor<String>(strTime) != benchCtor<std::string>(stdTime)) {
			fatal("Strings not equal !");
		}//printScore(strTime, stdTime);
		if(benchEq<String>(strTime) != benchEq<std::string>(stdTime)) {
			fatal("Strings not equal !");
		}//printScore(strTime, stdTime);
		if(benchCat<String>(strTime) != benchCat<std::string>(stdTime)) {
			fatal("Strings not equal !");
		}//printScore(strTime, stdTime);
		if(benchMulti<String>(strTime) != benchMulti<std::string>(stdTime)) {
			fatal("Strings not equal !");
		}//printScore(strTime, stdTime);
		if(!stdTime && !strTime) {
			return;
		}
	} while(stdTime + strTime < time);
}

void benchStrings() {
	double stdTime = 0;
	double strTime = 0;
	benchmark(stdTime, strTime);
	printScore(strTime, stdTime);
}



class BS {};


int main(int, char **) {
	script::DynamicBytecode bc[] = {
		script::DynamicBytecode(script::DynamicBytecode::Push, 9),
		script::DynamicBytecode(script::DynamicBytecode::Push, 17),
		script::DynamicBytecode(script::DynamicBytecode::SlB),

		script::DynamicBytecode(script::DynamicBytecode::Push, 18),
		script::DynamicBytecode(script::DynamicBytecode::Push, 19),
		script::DynamicBytecode(script::DynamicBytecode::SlE),
		script::DynamicBytecode(script::DynamicBytecode::Push, 646),
		script::DynamicBytecode(script::DynamicBytecode::Add),
		script::DynamicBytecode(script::DynamicBytecode::Cast, script::DynamicPrimitive(script::PrimitiveType::Array)),
		script::DynamicBytecode(script::DynamicBytecode::End)};


	script::DynamicPrimitive ret = script::Machine::run(bc);
	if(ret.type() != script::PrimitiveType::Array) {
		fatal("Expected Array");
	}
	Array<script::DynamicPrimitive> arr = ret.data().to<Array<script::DynamicPrimitive>>();
	std::cout<<"size = "<<arr.size()<<std::endl;
	for(script::DynamicPrimitive d : arr) {
		std::cout<<d.data().Int<<std::endl;
	}

	return 0;
}

