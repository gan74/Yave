#define ALL
#ifdef ALL
#include "main.h"

Console console;

ShaderCombinaison *createShader();
ShaderCombinaison *createLightShader();

int main(int, char **) {
	SDL_Window *win = createWindow();

	Camera cam;
	cam.setPosition(Vec3(10, 0, 0));
	cam.setRatio(4/3.0);
	cam.setForward(-cam.getPosition());
	//cam.setRotation(Quaternion<>::fromEuler(0, toRad(90), 0));

	Scene scene;
	scene.insert(&cam);

	auto c = new Obj("g2.obj");
	c->setPosition(Vec3(0, 0, 0));
	c->setAutoScale(6);
	scene.insert(c);

	Light *l = new Light();
	l->setPosition(Vec3(5, 5, 0));
	scene.insert(l);

	/*c = new Obj("scube.obj");
	c->setPosition(Vec3(0, 0, 0));
	c->setAutoScale(6);
	scene.insert(c);*/

	uint num = 1;
	uint count = console("$objCount").to<uint>([&](){ num = 0; });
	for(uint i = 0; i < (count * num); i++) {
		scene.insert(new RandObj());
	}

	FrameBufferRenderer renderer(new DeferredShadingRenderer(new GBufferRenderer(new SceneRenderer(&scene))));
	//ShaderRenderer renderer(new SceneRenderer(&scene), createShader());

	console.start();

	while(run(win)) {
		Timer timer;

		cam.setPosition(Quaternion<>::fromEuler(Vec3(0, mouse.y(), mouse.x()))(Vec3(10, 0, 0)));
		cam.setForward(-cam.getPosition());

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






ShaderCombinaison *createLightShader() {
	return 0;
}

ShaderCombinaison *createShader() {
	Shader<FragmentShader> *frag = new Shader<FragmentShader>("#version 420 core\n"
		"out vec4 color;"
		"uniform float width;"
		"in vec3 bc;"

		"in flat int id;"

		"float edgeFactor() {"
			"vec3 d = fwidth(bc);"
			"vec3 a3 = smoothstep(vec3(0.0), d * width, bc);"
			"return min(min(a3.x, a3.y), a3.z);"
		"}"

		"float vertexFactor() {"
			"vec3 a3 = bc;"
			"float m = max(max(a3.x, a3.y), a3.z);"
			"return a3.x + a3.y + a3.z - m;"
		"}"


		"void main() {"
			"color = vec4(vec3(mix(0.0, 0.7, edgeFactor())), 1.0);"
			/*"if(vertexFactor() < 0.1) {"
				"vec3 b = vec3((id >> 0) & 0xFF, (id >> 8) & 0xFF, (id >> 16) & 0xFF);"
				"color = vec4(vec3(b / vec3(0xFF, 16, 0xFF)), 1.0);"
			"}"*/
		"}"
	);

	Shader<GeometryShader> *geom = new Shader<GeometryShader>("#version 420 core\n"
		"layout(triangles) in;"
		"layout (triangle_strip, max_vertices = 3) out;"
		"in int n_VertexID[3];"
		"in vec3 n_Position[3];"
		"out vec3 bc;"
		"out flat int id;"
		"void main() {"
			"vec3 bcs[3] = vec3[3](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));"
			"for(int i = 0; i < gl_in.length(); i++) {"
				"gl_Position = gl_in[i].gl_Position;"
				"id = n_VertexID[i];"
				"bc = bcs[i];"
				"EmitVertex();"
			"}"
			"EndPrimitive();"
		"}"
	);

	ShaderCombinaison *shader = new ShaderCombinaison(frag, 0, geom);
	(*shader)["width"] = 1.0;
	std::cerr<<shader->getLogs()<<std::endl;
	return shader ;
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

