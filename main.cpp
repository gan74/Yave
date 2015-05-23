//#define ALL
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

	for(uint i = 0; i != 100; i++) {
		auto obj = new Obj("scube.obj");
		obj->setAutoScale(6);
		obj->setPosition(Vec3(random(), random(), random()) * 100 - 50);
		scene.insert(obj);
	}

	{
		PointLight<> *l = new PointLight<>();
		l->setPosition(Vec3(-5, 5, 10));
		l->setRadius(25);
		scene.insert(l);
	}
	{
		DirectionalLight<> *l = new DirectionalLight<>();
		l->setPosition(Vec3(-5, -5, 5));
		l->setColor(Color<>(Blue) * 2);
		scene.insert(l);
	}

	BufferedRenderer *ri = 0;
	ri = new DeferredShadingRenderer(new GBufferRenderer(new SceneRenderer(&scene)));
	FrameBufferRenderer renderer(ri);

	Timer timer;
	Timer total;

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

		/*if(total.elapsed() > 20) {
			break;
		}*/
	}
	return 0;
}


#else

#include <iostream>
#include <cstring>
#include <n/utils.h>
#include <n/core/Timer.h>
#include <n/core/String.h>


using namespace n;
using namespace n::core;
using namespace n::io;

int main(int, char **) {
	String str = "utfvybpouvbsdpovbqdspdf";
	uint max = 1 << 24;
	uint p = 0;
	std::cout<<"Generating ...."<<std::endl<<"0%";
	while(str.size() < max) {
		str += char('a' + rand() % 26);
		uint p2 = round(double(str.size()) / max * 100);
		if(p2 != p) {
			std::cout<<"\r"<<p2<<"%";
			p = p2;
		}
	}
	std::cout<<std::endl;
	Timer timer;
	uint64 h = hash(&str[0], str.size());
	double t = timer.elapsed();
	std::cout<<"hash = "<<h<<" ("<<t * 1000<<"ms : "<<str.size() / t<<" bps)"<<std::endl;
}

#endif

