#include "main.h"

int main(int argc, char **argv) {

	SDL_Window *win = createWindow();

	if(argc > 1 && argv[1] == String("--no-debug")) {
		GLContext::getContext()->setDebugEnabled(false);
	}

	PerspectiveCamera cam;
	cam.setPosition(Vec3(-25, 0, 25));
	cam.setRatio(4/3.0);
	cam.setForward(-cam.getPosition());


	Light *light = 0;
	Obj *tr;

	Scene scene;
	scene.insert(&cam);


	for(uint i = 0; i != 15; i++) {
		auto obj = new Obj("scube.obj");
		obj->setAutoScale(5);
		obj->setPosition(Vec3(0, 0, 5) * i);
		scene.insert(tr = obj);
	}

	{
		auto obj = new Obj("./crytek-sponza/sponza.obj");
		obj->setRotation(Quaternion<>::fromEuler(0, 0, pi * 0.5));
		obj->setAutoScale(800);
		scene.insert(obj);
	}

	{
		auto obj = new Obj("plane.obj");
		obj->setAutoScale(800);
		obj->setPosition(Vec3(0, 0, -5));
		scene.insert(obj);
	}

	{
		BoxLight *l = new BoxLight(600);
		l->setForward(Vec3(0, 0, -1));
		l->setPosition(Vec3(0, 0, 10));
		l->setIntensity(5);
		//l->setCastShadows<VarianceShadowRenderer>(&scene, 1024, 2);
		scene.insert(l);
	}


	SceneRenderer *sceRe = new SceneRenderer(&scene);
	GBufferRenderer *gRe = new GBufferRenderer(sceRe);
	DeferredShadingRenderer *ri = new DeferredShadingRenderer(gRe);
	Renderer *renderers[] {new FrameBufferRenderer(ri),
						   new FrameBufferRenderer(gRe, 0),
						   new FrameBufferRenderer(gRe, 1),
						   new FrameBufferRenderer(gRe, 2),
						   tone = new BasicToneMapRenderer(ri)};

	Timer timer;
	Timer total;

	uint64 frames = 0;

	double totalCpu = 0;
	uint totalFrames = 0;
	int skipped = 0;
	std::cout<<" ";


	while(run(win)) {
		Timer frame;
		if(!bench) {
			frames = 0;
			double dt = timer.reset();
			cam.setPosition(cam.getPosition() + (wasd.x() * cam.getForward() + wasd.y() * cam.getTransform().getY()) * dt * 100);
			Vec2 angle = mouse * 0.01;
			float p2 = pi * 0.5 - 0.01;
			angle.y() = std::min(std::max(angle.y(), -p2), p2);
			Vec3 f = Vec3(Vec2(cos(-angle.x()), sin(-angle.x())) * cos(angle.y()), -sin(angle.y()));
			cam.setForward(f);
			float tt = total.elapsed() * 0.15;
			if(light) {
				light->setForward(Vec3(0, cos(tt), -fabs(sin(tt)) - 2));
			}
		} else {
			double tt = timer.elapsed();
			frames++;
			float ang = tt * 0.33;
			cam.setPosition(Vec3(cos(ang) * 350, sin(ang) * 100, 185));
			cam.setForward(Vec3(-cos(ang), -sin(ang), -0.2));
			if(tt > 60) {
				bench = false;
				std::cout<<"\r"<<frames<<" frames in "<<tt<<" seconds ("<<frames / tt<<" fps, "<<tt / frames * 1000<<"ms)"<<std::endl<<" ";
			}
			if(light) {
				light->setForward(Vec3(0, 0.15, -1));
			}
		}

		uint rCount = sizeof(renderers) / sizeof(void *);
		bool warp = rendererIndex >= rCount;
		uint rIndex = warp ? 0 : rendererIndex;
		uint dIndex = ((std::max(rCount, rendererIndex) - rCount) % (DeferredShadingRenderer::Max - warp)) + warp;
		ri->setDebugMode(DeferredShadingRenderer::LightingDebugMode(dIndex));
		(*renderers[rIndex])();

		GLContext::getContext()->processTasks();
		GLContext::getContext()->flush();





		double frameTime = frame.elapsed() * 1000;
		double thisFrame = frameTime;
		if(skipped > 10) {
			totalFrames = 0;
			totalCpu = 0;
			skipped = 0;
		}
		if(totalFrames > 10) {
			double diff = 1.0;
			if(totalFrames > 20) {
				diff = thisFrame / (totalCpu / (totalFrames - 10));
				diff = std::max(diff, 1.0 / diff);
			}
			if(diff >= 1.5) {
				skipped++;
			} else {
				totalCpu += thisFrame;
				totalFrames++;
				skipped = std::max(skipped - 2, 0);
			}
		} else {
			totalFrames++;
		}
		std::cout<<"\rcpu = "<<(String(frameTime) + "0000").subString(0, 4)<<" ms (avg = "<<(String(totalCpu / (totalFrames - 10)) + "0000").subString(0, 4)<<" ms)";

		GLContext::getContext()->fatalIfError();
	}
	return 0;
}
