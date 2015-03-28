#ifndef N_CONCURENT_ATOMIC
#define N_CONCURENT_ATOMIC

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


template<typename T>
class AtomicAssignable : private std::atomic<T>
{
	static_assert(std::is_integral<T>::value || std::is_pointer<T>::value, "Atomic type should be integral or pointer");

	public:
		AtomicAssignable(const T &t = T()) : std::atomic<T>(t) {
		}

		AtomicAssignable(const AtomicAssignable<T> &t) : std::atomic<T>(static_cast<std::atomic<T>>(t)) {
		}

		T operator=(const T &t) {
			this->store(t);
			return t;
		}

		T operator=(const T &t) volatile {
			this->store(t);
			return t;
		}

		operator T() const {
			return this->load();
		}

		T operator=(const AtomicAssignable<T> &t) {
			return std::atomic<T>::operator=(static_cast<std::atomic<T>>(t));
		}
};

}
}

#endif // N_CONCURENT_ATOMIC

