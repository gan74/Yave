#define ALL
#ifdef ALL
#include "main.h"

ShaderCombinaison *createShader();
ShaderCombinaison *createNoiseShader();

int main(int, char **) {
	SDL_Window *win = createWindow();

	Camera cam;
	cam.setPosition(Vec3(10, 0, 0));
	cam.setRatio(4/3.0);
	cam.setForward(-cam.getPosition());
	//cam.setRotation(Quaternion<>::fromEuler(0, toRad(90), 0));

	Scene scene;
	scene.insert(&cam);

	auto c = new Obj("scube.obj");
	c->setPosition(Vec3(0, 0, 0));
	c->setAutoScale(6);
	scene.insert(c);

	Light *l = new Light();
	l->setPosition(Vec3(0, 0, -5));
	scene.insert(l);
	l = new Light();
	l->setPosition(Vec3(0, 0, 5));
	scene.insert(l);

	//FrameBufferRenderer renderer(new DeferredShadingRenderer(new GBufferRenderer(new SceneRenderer(&scene))));
	ShaderRenderer renderer(new ScreenQuadRenderer(), createNoiseShader());

	while(run(win)) {

		c->setRotation(c->getRotation() * Quaternion<>::fromAxisAngle(Vec3(0, 0, 1), dMouse.x()) * Quaternion<>::fromAxisAngle(Vec3(0, 1, 0), dMouse.y()));

		renderer();

		GLContext::getContext()->finishTasks();
		GLContext::getContext()->flush();

		if(GLContext::getContext()->checkGLError()) {
			fatal("OpenGL error");
		}
	}

	return 0;
}






ShaderCombinaison *createNoiseShader() {
	Shader<FragmentShader> *frag = new Shader<FragmentShader>("#version 400\n"
		"float rand(vec2 co) {"
			"return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);"
		"}"

		"vec2 grd(vec2 c) {"
			"float gx = rand(c);"
			"return vec2(gx, 1.0 - gx * gx);"
		"}"

		"float dotGrd(vec2 p, ivec2 c) {"
			"return dot(grd(vec2(c)) * 2 - 1, p - vec2(c));"
		"}"

		"float fade(float x) {"
			"return x * x * (3.0 - 2.0 * x);"
		"}"

		"vec2 fade(vec2 x) {"
			"return vec2(fade(fade(x.x)), fade(fade(x.y)));"
		"}"

		"float cnoise(vec2 p) {"
			"ivec2 cell = ivec2(floor(p));"
			"ivec2 x0y0 = cell;"
			"ivec2 x1y0 = cell + ivec2(1, 0);"
			"ivec2 x0y1 = cell + ivec2(0, 1);"
			"ivec2 x1y1 = cell + ivec2(1, 1);"
			"float s = dotGrd(p, x0y0);"
			"float t = dotGrd(p, x1y0);"
			"float u = dotGrd(p, x0y1);"
			"float v = dotGrd(p, x1y1);"
			"vec2 faded = fade(fract(p));"
			"vec2 f2 = mix(vec2(s, u), vec2(t, v), faded.x);"
			"float f = mix(f2.x, f2.y, faded.y);"
			"return f + 0.5;"
		"}"

		"uniform float scale;"
		"uniform int d;"
		"uniform float br;"
		"in vec2 n_TexCoord;"
		"out vec4 color;"

		"void main() {"
			"float tot = 0;"
			"float n = 0;"
			"for(int i = 0; i < d; i++) {"
				"float w = pow(0.5, i);"
				"n += w * cnoise(n_TexCoord * (1 + scale) / w);"
				"tot += w;"
			"}"
			"n /= tot;"
			"float r = rand(n_TexCoord);"
			"color = vec4(mix(n, r, br));"
		"}");

	ShaderCombinaison *shader = new ShaderCombinaison(frag, ShaderCombinaison::NoProjectionShader);
	std::cerr<<shader->getLogs()<<std::endl;
	(*shader)["scale"] = 2.464;
	(*shader)["d"] = 5;
	(*shader)["br"] = 0.02;
	return shader ;
}

ShaderCombinaison *createShader() {
	Shader<FragmentShader> *frag = new Shader<FragmentShader>(
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
				"color = vec4(vec3(b / vec3(0xwFF, 16, 0xFF)), 1.0);"
			"}"*/
		"}"
	);

	Shader<GeometryShader> *geom = new Shader<GeometryShader>(
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

