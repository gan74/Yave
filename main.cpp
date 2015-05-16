#define ALL
#ifdef ALL
#include "main.h"

int main(int argc, char **argv) {
	SDL_Window *win = createWindow();

	if(argc > 1 && argv[1] == String("--no-debug")) {
		GLContext::getContext()->setDebugEnabled(false);
	}

	Camera<> cam;
	cam.setPosition(Vec3(-10, 0, 10));
	cam.setRatio(4/3.0);
	cam.setForward(-cam.getPosition());

	Scene<> scene;
	scene.insert(&cam);

	auto obj = new Obj("scube.obj");
	obj->setAutoScale(6);
	obj->setPosition(Vec3(0, 0, 0));
	scene.insert(obj);

	{
		PointLight<> *l = new PointLight<>();
		l->setPosition(Vec3(-5, 5, 10));
		l->setRadius(25);
		scene.insert(l);
	}
	{
		DirectionalLight<> *l = new DirectionalLight<>();
		l->setPosition(Vec3(-5, -5, 5));
		l->setColor(Color<>(Blue));
		scene.insert(l);
	}

	BufferedRenderer *ri = 0;
	ri = new DeferredShadingRenderer(new GBufferRenderer(new SceneRenderer(&scene)));
	FrameBufferRenderer renderer(ri);

	Timer timer;

	while(run(win)) {
		double dt = timer.reset();
		cam.setPosition(cam.getPosition() + (wasd.x() * cam.getForward() + wasd.y() * cam.getTransform().getY()) * dt * 50);
		Vec2 angle = mouse * 0.01;
		float p2 = pi<>() * 0.5 - 0.01;
		angle.y() = std::min(std::max(angle.y(), -p2), p2);
		cam.setForward(Vec3(Vec2(cos(angle.x()), sin(angle.x())) * cos(angle.y()), -sin(angle.y())));

		(renderer)();

		GLContext::getContext()->finishTasks();
		GLContext::getContext()->flush();

		if(GLContext::getContext()->checkGLError()) {
			fatal("OpenGL error");
		}
	}
	return 0;
}


#else

#include <iostream>
#include <n/core/Timer.h>
#include <n/io/File.h>
#include <n/io/TextInputStream.h>
#include <n/core/String.h>
#include <n/core/Array.h>
#include <n/core/Functor.h>
#include <n/math/Vec.h>
#include <n/mem/SmallObject.h>
#include <n/concurrent/SpinLock.h>
#include <n/concurrent/Async.h>
#include <n/concurrent/Mutex.h>

using namespace n;
using namespace n::io;

int main(int, char **) {
}

#endif

