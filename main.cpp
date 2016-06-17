
#include <iostream>
#include <n/concurrent/Thread.h>

using namespace n;
using namespace n::concurrent;


class Test : public Thread
{
	public:
		virtual void run() override {
				sleep(5);
		}
};

int main(int, char **) {
	Test t;
	t.start();
	for(uint i = 0; i != 3; i++) {
		std::cout << t.isRunning() << std::endl;
		Thread::sleep(1);
	}
	t.join();
	for(uint i = 0; i != 3; i++) {
		std::cout << t.isRunning() << std::endl;
		Thread::sleep(1);
	}
	return 0;
}
