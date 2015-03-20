#define ALL
#ifdef ALL
#include "main.h"

Console console;
static_assert(sizeof(void *) == 4, "compiler should be 32bits");

int main(int, char **) {
	SDL_Window *win = createWindow();

	Camera cam;
	cam.setPosition(Vec3(0, 0, 5));
	cam.setRatio(4/3.0);
	cam.setRotation(Quaternion<>::fromEuler(0, toRad(90), 0));

	GLContext::getContext()->setProjectionMatrix(cam.getProjectionMatrix());
	GLContext::getContext()->setViewMatrix(cam.getViewMatrix());

	Scene scene;
	scene.insert(&cam);

	auto c = new Obj("MP5_Scene.obj");
	c->setPosition(Vec3(0, 0, 0));
	c->setScale(10);
	scene.insert(c);

	DynamicBuffer<int> buff(10000);
	for(int &i : buff) {
		i = 7;
	}

	VertexAttribs attribs;
	attribs.addAttrib(4, &buff);
	c->setAttribs(attribs);

	uint num = 1;
	uint count = console("$objCount").to<uint>([&](){ num = 0; });
	for(uint i = 0; i < (count * num); i++) {
		scene.insert(new RandObj());
	}

	FrameBufferRenderer renderer(new GBufferRenderer(new SceneRenderer(&scene)));


	console.start();

	while(run(win)) {
		Timer timer;

		renderer();

		GLContext::getContext()->finishTasks();
		GLContext::getContext()->flush();

		if(GLContext::getContext()->checkGLError()) {
			fatal("OpenGL error");
		}

		float fps = float(console("$framerate"));
		if(fps) {
			fps = (1.0 / fps);
			fps -= timer.elapsed();
			if(fps > 0.0) {
				concurrent::Thread::sleep(fps);
			}
		}
	}

	return 0;
}


#else

#include <iostream>
#include <n/core/Timer.h>
#include <n/core/Array.h>
#include <n/core/Functor.h>
#include <n/mem/SmallObject.h>
#include <n/concurrent/SpinLock.h>
#include <n/concurrent/Async.h>
#include <n/concurrent/Mutex.h>

using namespace n;
using namespace n::core;
using namespace n::concurrent;

class I : public mem::SmallObject<I>
{
	public:
		I(int x) : i(x) {
		}

		const int i;
};

class W
{
	public:
		void *operator new(uint size) {
			return malloc(size);
		}

};

class J : public W
{
	public:
		J(int x) : i(x) {
		}

		const int i;
		};

int main(int, char **) {
	SpinLock lock;
	Array<SharedFuture<void>> futures;
	Timer ti;
	uint threads = 10;
	uint max = 1000000;
	for(uint i = 0; i != threads; i++) {
		futures += Async([=, &lock]() {
			for(uint j = 0; j != max; j++) {
				lock.lock();
				lock.unlock();
			}
		});
	}
	for(cnst SharedFuture<void> &f : futures) {
		f.wait();
	}
	std::cout<<ti.reset() * 1000<<"ms"<<std::endl;
	for(uint j = 0; j != max * threads; j++) {
		lock.lock();
		lock.unlock();
	}
	std::cout<<ti.reset() * 1000<<"ms"<<std::endl;


}

#endif

