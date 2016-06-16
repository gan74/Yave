#ifndef N_CONCURRENT_ATOMIC_H
#define N_CONCURRENT_ATOMIC_H

#include <n/types.h>
#include <atomic>

namespace n {
namespace concurrent {

template<typename T>
class Atomic : public std::atomic<T>
{
	static_assert(std::is_integral<T>::value || std::is_pointer<T>::value, "Atomic type should be integral or pointer");

	public:
		template<typename... Args>
		Atomic(const T &t = T()) : std::atomic<T>(t) {
		}

		T operator=(const T &t) {
			this->store(t);
			return t;
		}

		T operator=(const T &t) volatile {
			this->store(t);
			return t;
		}
};

typedef Atomic<uint> auint;
typedef Atomic<bool> abool;

template<typename T>
using AtomicPtr = Atomic<T *>;

}
}

#endif // N_CONCURRENT_ATOMIC_H

