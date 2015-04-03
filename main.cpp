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

	auto c = new Obj(MeshInstance<>(std::move(TriangleBuffer<>::getSphere())));
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
	Shader<FragmentShader> *frag = new Shader<FragmentShader>("#version 420 core\n"
		"vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }"
		"vec2 fade(vec2 t) { return t*t*t*(t*(t*6.0-15.0)+10.0); }"
		"vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }"
		"vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }"
		"float cnoise(vec2 P) {"
			"vec4 iP = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);"
			"vec4 fP = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);"
			"iP = mod289(iP);"
			"vec4 ix = iP.xzxz;"
			"vec4 iy = iP.yyww;"
			"vec4 fx = fP.xzxz;"
			"vec4 fy = fP.yyww;"
			"vec4 i = permute(permute(ix) + iy);"
			"vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;"
			"vec4 gy = abs(gx) - 0.5 ;"
			"vec4 tx = floor(gx + 0.5);"
			"gx = gx - tx;"
			"vec2 g00 = vec2(gx.x,gy.x);"
			"vec2 g10 = vec2(gx.y,gy.y);"
			"vec2 g01 = vec2(gx.z,gy.z);"
			"vec2 g11 = vec2(gx.w,gy.w);"
			"vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));"
			"g00 *= norm.x;"
			"g01 *= norm.y;"
			"g10 *= norm.z;"
			"g11 *= norm.w;"
			"float n00 = dot(g00, vec2(fx.x, fy.x));"
			"float n10 = dot(g10, vec2(fx.y, fy.y));"
			"float n01 = dot(g01, vec2(fx.z, fy.z));"
			"float n11 = dot(g11, vec2(fx.w, fy.w));"
			"vec2 fade_xy = fade(fP.xy);"
			"vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);"
			"float n_xy = mix(n_x.x, n_x.y, fade_xy.y);"
			"return (2.3 * n_xy) * 0.5 + 0.5;"
		"}"

		"float rand(vec2 co) {"
			"return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);"
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
				"color = vec4(vec3(b / vec3(0xwFF, 16, 0xFF)), 1.0);"
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

