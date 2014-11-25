#ifndef N_CONCURENT_SPINLOCK_H
#define N_CONCURENT_SPINLOCK_H

#include <pthread.h>
#include <n/utils.h>

namespace n {
namespace concurent {

class SpinLock : public core::NonCopyable
{
	public:
		SpinLock();

		void lock();
		void unlock();
		bool trylock();

	private:
		pthread_spinlock_t spin;
};


}
}

#endif // N_CONCURENT_SPINLOCK_H
