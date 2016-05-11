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

	io::File file("settings.dat");
	if(file.open(io::IODevice::Read | io::IODevice::Binary)) {
		Vec3 pos;
		file.readBytes(&pos, sizeof(Vec3));
		file.readBytes(&mouse, sizeof(Vec2));
		cam.setPosition(pos);
		file.close();
	}

	Scene scene;
	scene.insert(&cam);

	uint max = 20;
	float scale = 5;
	for(uint i = 0; i != max; i++) {
		for(uint j = 0; j != max; j++) {
			auto m = new MaterialTest(i / float(max), j / float(max));
			m->setScale(scale);
			m->setPosition(Vec3(i - max * 0.5, j - max * 0.5, 1) * m->getRadius() * 2.5);
			scene.insert(m);
		}
	}

	/*{
		auto obj = new Obj("./crytek-sponza/sponza.obj");
		obj->setRotation(Quaternion<>::fromEuler(0, 0, pi * 0.5));
		//auto obj = new Obj("plane.obj");
		obj->setAutoScale(800);
		scene.insert(obj);
	}*/

	{
		DirectionalLight *l = new DirectionalLight();

		l->setForward(Vec3(0, 1, -1));
		l->setIntensity(5);
		scene.insert(l);
	}

	{
		DirectionalLight *l = new DirectionalLight;

		l->setForward(Vec3(0, -1, -1));
		l->setIntensity(1);
		l->setColor(BaseColor::Blue);
		scene.insert(l);
	}

	/*for(uint i = 0; i != 1000; i++) {
		PointLight *l = new PointLight;
		l->setIntensity(5);
		l->setRadius(1);
		l->setPosition(Vec3((Vec2(random(), random()) * 2.0 - 1.0) * 400, 1));
		l->setScale(random() * 150 + 75);
		//l->setScale(150);

		l->setColor(BaseColor(random(1, BaseColor::Blue + 1)));
		scene.insert(l);
	}*/

	/*for(uint i = 0; i != 1; i++) {
		Test *t = new Test(MaterialLoader::load<String>("test.nmt"));
		scene.insert(t);
	}*/

	core::Array<ParticleEmitter *> emitters;
	uint parts = ParticleEmitter::getMaxParticles();
	for(uint i = 0; i != 1; i++) {
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
	TiledDeferredShadingRenderer *ti = new TiledDeferredShadingRenderer(gRe);
	Renderer *renderers[] {new DeferredIBLRenderer(gRe),
						   new FrameBufferRenderer(ti),
						   new FrameBufferRenderer(ri),
						   sceRe,
						   new FrameBufferRenderer(gRe),
						   new FrameBufferRenderer(gRe, 1),
						   new FrameBufferRenderer(gRe, 2)};

	String renderNames[] = {"IBL",
							"Tiled shading",
							"Deferred shading",
							"Scene",
							"G-buffer 0",
							"G-buffer 1",
							"G-buffer 2"};

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
	} catch(std::exception &e) {
		logMsg("Exception caught.", ErrorLog);
		logMsg(e.what(), ErrorLog);
	}

	file.open(io::IODevice::Write | io::IODevice::Binary);
	file.writeBytes(&cam.getPosition(), sizeof(Vec3));
	file.writeBytes(&mouse, sizeof(Vec2));
	file.close();

	return 0;
}


#else

#include <iostream>
#include <n/core/String.h>
#include <n/io/File.h>
#include <n/io/Dir.h>
#include <n/core/Timer.h>

using namespace n;
using namespace n::math;
using namespace n::core;

/*Array<String> readLines(String f) {
	io::File file(f);
	if(!file.open(io::IODevice::Read)) {
		std::cerr<<"ERROR : unable to open : "<<f<<std::endl;
		return {};
	}
	char *data = new char[file.size() + 1];
	data[file.readBytes(data)] = 0;
	Array<String> lines = String(data).split("\n");
	delete[] data;
	file.close();
	return lines;
}

void checkLen(String f, const Array<String> &lines) {
	for(uint i = 0; i != lines.size(); i++) {
		const String &l = lines[i];
		uint tabs = AsCollection(l).count('\t');
		uint len = l.size() + tabs * 3;
		if(len > 85) {
			std::cout<<f<<" at line "<<i<<" is too long."<<std::endl;
		}

	}

}

void checkFuncs(String f, const Array<String> &lines) {
	int o = 0;
	uint beg = 0;
	for(uint i = 0; i != lines.size(); i++) {
		const String &l = lines[i];
		uint op = AsCollection(l).count('{');
		uint cl = AsCollection(l).count('}');
		int lo = o;
		o = o + op - cl;
		if(o && !lo) {
			beg = i;
		}
		if(!o && lo) {
			uint len = i - beg;
			if(len > 120) {
				std::cout<<f<<" at line "<<beg<<" function is too long."<<std::endl;
			}
		}
	}

}

void parseh(String f) {
	auto lines = readLines(f);
	//std::cout<<f<<" has "<<lines.size()<<" lines"<<std::endl;
	checkLen(f, lines);
	for(auto l : lines) {
		if(l.contains("{")) {
			std::cout<<"\"{\" in "<<f<<std::endl;
		}
	}
}


void parsec(String f) {
	auto lines = readLines(f);
	//std::cout<<f<<" has "<<lines.size()<<" lines"<<std::endl;
	checkLen(f, lines);
	checkFuncs(f, lines);

}

int main(int, char **argv) {
	if(!io::Dir::exists(argv[1])) {
		std::cerr<<argv[1]<<" does not exists"<<std::endl;
		return 1;
	}
	io::Dir dir(argv[1]);
	for(auto f : dir.getContent()) {
		if(f.endsWith(".h")) {
			parseh(dir.getPath() + "\\" + f);
		} else if(f.endsWith(".c") || f.endsWith(".cpp")) {
			parsec(dir.getPath() + "\\" + f);
		}
	}

	std::cout<<"DONE"<<std::endl;
	return 0;
}*/

class Test {

	public:
		Test() : a(7) {
			std::cout<<"truc("<<this<<")"<<std::endl;
		}

		int a;

};

int main(int, char **) {
	new(0) Test();
	return 0;
}

#endif

