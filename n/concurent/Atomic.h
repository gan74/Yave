#ifndef N_CONCURENT_ATOMIC
#define N_CONCURENT_ATOMIC

#include <n/types.h>
#include <atomic>

namespace n {
namespace concurent {

template<typename T>
class Atomic : public std::atomic<T>
{
	static_assert(std::is_integral<T>::value, "Atomic type should be integral");

	public:
		template<typename... Args>
		Atomic(Args... args) : std::atomic<T>(args...) {
		}

		T operator=(const T &t) {
			store(t);
			return t;
		}

		T operator=(const T &t) volatile {
			this->store(t);
			return t;
		}
};

typedef Atomic<uint> auint;
typedef Atomic<bool> abool;

}
}

#endif // N_CONCURENT_ATOMIC

