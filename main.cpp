//#define ALL
#ifdef ALL
#include "main.h"

int main(int, char **) {
	SDL_Window *win = createWindow();

	Camera cam;
	cam.setPosition(Vec3(-10, 0, 10));
	cam.setRatio(4/3.0);
	cam.setForward(-cam.getPosition());

	Scene scene;
	scene.insert(&cam);

	auto c = new PerlinTerrain();
	//auto c = new Obj("scube.obj");
	//c->setAutoScale(6);
	c->setPosition(Vec3(0, 0, 0));
	scene.insert(c);

	Light *l = new Light();
	l->setPosition(Vec3(0, 0, 5));
	scene.insert(l);

	BufferedRenderer *ri = 0;
	//ri = new GBufferRenderer(new SceneRenderer(&scene));
	ri = new DeferredShadingRenderer(new GBufferRenderer(new SceneRenderer(&scene)));
	//ri = new ScreenShaderRenderer(createNoiseShader());
	//FrameBufferRenderer renderer(createBlurRenderer(ri));
	FrameBufferRenderer renderer(ri);

	while(run(win)) {

		c->setRotation(Quaternion<>::fromEuler(mouse.x(), 0, 0));

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

