#define ALL
#ifdef ALL
#include "main.h"

int main(int argc, char **argv) {
	io::File *traceFile = new io::File("./perfdump.json");
	traceFile->open(io::IODevice::Write);
	setTraceOutputStream(traceFile);


	SDL_Window *win = createWindow();

	if(argc > 1 && argv[1] == String("--no-debug")) {
		GLContext::getContext()->setDebugEnabled(false);
	}

	PerspectiveCamera cam;
	cam.setPosition(Vec3(-25, 0, 25));
	cam.setRatio(16.0 / 9.0);
	cam.setForward(-cam.getPosition());

	Scene scene;
	scene.insert(&cam);

	uint max = 10;
	float scale = 5;
	for(uint i = 0; i != max; i++) {
		for(uint j = 0; j != max; j++) {
			auto m = new MaterialTest(i / float(max), j / float(max));
			m->setScale(scale);
			m->setPosition(Vec3(i - max * 0.5, j - max * 0.5, -1) * m->getRadius() * 2.5);
			scene.insert(m);
		}
	}

	{
		auto obj = new Obj("./crytek-sponza/sponza.obj");
		obj->setRotation(Quaternion<>::fromEuler(0, 0, pi * 0.5));
		//auto obj = new Obj("plane.obj");
		obj->setAutoScale(800);
		//	scene.insert(obj);
	}

	{
		//BoxLight *l = new BoxLight(600);
		DirectionalLight *l = new DirectionalLight();

		l->setForward(Vec3(0, 1, -1));
		l->setIntensity(5);
		//l->setCastUnfilteredShadows(&scene, 1024);
		scene.insert(l);
	}

	{
		DirectionalLight *l = new DirectionalLight;

		l->setForward(Vec3(0, -1, -1));
		l->setIntensity(1);
		l->setColor(BaseColor::Blue);
		scene.insert(l);
	}

	core::Array<ParticleEmitter *> emitters;
	uint parts = ParticleEmitter::getMaxParticles();
	for(uint i = 0; i != 1; i++) {
		class TestModifier : public ParticleEmitter::Modifier
		{
			public:
				virtual void modify(Particle &p, double dt) override {
					Vec3 v = p.position - Vec3(0, 10, 10);
					p.velocity -= (v / v.length2()) * dt * 625; // v / r not r²
				}
		};

		ParticleEmitter *particles = new ParticleEmitter(parts);
		particles->setVelocityDistribution(new UniformVec3Distribution<>(Vec3(0, 0, 1), pi, 25, 50));
		particles->setSizeOverLife(PrecomputedRange<Vec2>(Array<Vec2>({Vec2(0.01, 0.0025), Vec2(0.015, 0.0025), Vec2(0)})));
		particles->setLifeDistribution(new UniformDistribution<float>(5, 10));
		//particles->setAcceleration(Vec3(0, 0, -9.8));
		particles->setFlow(parts / 5.0);
		particles->setFlags(ParticleEmitter::VelocityOriented | ParticleEmitter::VelocityScaled);
		//particles->setDrag(0.1);
		particles->addModifier(new TestModifier());
		scene.insert(particles);
		emitters.append(particles);
	}



	SceneRenderer *sceRe = new SceneRenderer(&scene);
	GBufferRenderer *gRe = new GBufferRenderer(sceRe);
	DeferredShadingRenderer *ri = new DeferredShadingRenderer(gRe);
	Renderer *renderers[] {new FrameBufferRenderer(ri),
						   sceRe,
						   new FrameBufferRenderer(gRe),
						   new FrameBufferRenderer(gRe, 1),
						   new FrameBufferRenderer(gRe, 2),
						   new DeferredIBLRenderer(gRe),
						   tone = new BasicToneMapRenderer(ri)};

	String renderNames[] = {"Deferred shading",
							"Scene",
							"G-buffer 0",
							"G-buffer 1",
							"G-buffer 2",
							"IBL",
							"Tone mapped"};

	Timer timer;

	try {
	while(run(win)) {
		double dt = timer.reset();
		cam.setPosition(cam.getPosition() + (wasd.x() * cam.getForward() + wasd.y() * cam.getTransform().getY()) * dt * 100);
		Vec2 angle = mouse * 0.01;
		float p2 = pi * 0.5 - 0.01;
		angle.y() = std::min(std::max(angle.y(), -p2), p2);
		Vec3 f = Vec3(Vec2(cos(-angle.x()), sin(-angle.x())) * cos(angle.y()), -sin(angle.y()));
		cam.setForward(f);

		uint count = sizeof(renderers) / sizeof(void *);
		uint index = rendererIndex % count;
		(*renderers[index])();
		SDL_SetWindowTitle(win, renderNames[index].toChar());

		GLContext::getContext()->finishTasks();
		GLContext::getContext()->flush();

		emitters.foreach([=](ParticleEmitter *p) { p->update(dt); });
	}
	} catch(...) {
		logMsg("Exception caught.", ErrorLog);
	}

	return 0;
}


#else

#include <iostream>
#include <n/core/Timer.h>
#include <n/math/Interpolator.h>

using namespace n;
using namespace n::math;
using namespace n::core;

int main(int, char **) {
	for(uint points = 0; points != 100; points++) {
		Array<LinearInterpolator<Vec2>::Element> pts;
		for(uint i = 0; i != points + 1; i++) {
			pts.append(LinearInterpolator<Vec2>::Element(i / float(points), Vec2(0.5, 0.5)));
		}
		LinearInterpolator<Vec2> inter(pts);
		PrecomputedDistribution<Vec2> distr = inter.toDistribution();


		uint max = 1000000;
		core::Timer timer;

		Vec2 sum2 = 0.0;
		for(uint i = 0; i != max + 1; i++) {
			sum2 += distr(i / float(max));
		}
		double t2 = timer.reset();

		Vec2 sum1 = 0.0;
		for(uint i = 0; i != max + 1; i++) {
			sum1 += inter.eval(i / float(max));
		}
		double t1 = timer.reset();

		std::cout<<std::endl<<points + 1<<" control points"<<std::endl;
		std::cout<<"Interpolator = "<<sum1<<" ("<<t1 * 1000<<"ms)"<<std::endl;
		std::cout<<"Distribution = "<<sum2<<" ("<<t2 * 1000<<"ms)"<<std::endl;
		std::cout<<"Distribution size = "<<distr.size()<<std::endl;
	}

	return 0;
}

#endif

