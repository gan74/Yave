
#include <n/types.h>

#include <n/core/Array.h>
#include <n/core/String.h>
#include <n/core/Set.h>
#include <n/core/Timer.h>
#include <n/test/Test.h>
#include <n/core/Map.h>
#include <n/core/Functor.h>
#include <n/core/SmartPtr.h>
#include <n/core/Lazy.h>

#include <n/concurent/Thread.h>
#include <n/concurent/Mutex.h>
#include <n/concurent/Promise.h>
#include <n/concurent/Async.h>
#include <n/concurent/ThreadPool.h>

#include <n/io/ConsoleStream.h>

#include <n/math/Vec.h>
#include <n/math/Matrix.h>

#include <iostream>
#include <set>
#include <assert.h>

using namespace	n::concurent;
using namespace n::core;
using namespace n::math;
using namespace n::io;
using namespace n;

template<typename A, typename B>
auto truc(A a, B b) -> decltype(a + b) { return a + b; }

template<typename T>
bool isProbablePrime(T p) {
	T a = (p + 1) / 2;
	return modPow<T>(a, p - 1, p);
}

template<typename T>
bool isPrime(T p) {
	T s = isqrt(p + 1) + 1;
	for(T i = 2; i < s; i++) {
		if(p % i == 0) {
			return false;
		}
	}
	return true;
}

template<typename T>
uint primes(T max) {
	Array<T> primes(2);
	for(uint i = 3; i != max; i += 2) {
		if(isPrime(i)) {
			primes.append(i);
			//std::cout<<i<<std::endl;
		}
	}
	return primes.size();
}

template<typename T>
uint fastPrimes(T max) {
	Array<T> primes(2);
	for(T i = 3; i != max; i += 2) {
		if(isProbablePrime(i) && isPrime(i)) {
			primes.append(i);
			//std::cout<<i<<std::endl;
		}
	}
	return primes.filtered([](T x) { return isPrime(x); }).size();
}

template<typename T>
Array<int> foo(const T &t) {
	return AsCollection(t).sub(t.begin(), t.end()).mapped([](typename Collection<T>::ElementType t) -> int { return t - 1; });
}

class NC : public NonCopyable
{
	public:
		NC() {
		}
};

template<typename T, uint N, uint M>
String m2s(Matrix<T, N, M> m) {
	String str;
	for(uint i = 0; i != N; i++) {
		str += "| ";
		for(uint j = 0; j != M; j++) {
			str += m[i][j] + String(" ");
		}
		str += " |\n";
	}
	return str;
}

int main(int, char **) {
	std::cout<<std::boolalpha<<(Array<int>(1, 2, 3, 4, 5, 6, 7, 8, 9) == Array<int>(1, 2, 3, 4, 5, 6, 7, 8, 9))<<std::endl;
	for(int w : foo(Array<int>(1, 2, 3, 4, 5, 6, 7, 8, 9))) {
		std::cout<<w<<std::endl;
	}
}



