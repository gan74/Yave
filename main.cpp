//#define ALL
#ifdef ALL
#include "main.h"

ShaderCombinaison *createNoiseShader();
ShaderCombinaison *createBlurShader(Vec2 dir);
ShaderCombinaison *createUBlurShader(Vec2 dir);
ShaderCombinaison *createRecBlurShader();
BufferedRenderer *createBlurRenderer(BufferedRenderer *r);
BufferedRenderer *createUBlurRenderer(BufferedRenderer *r);
BufferedRenderer *createRecBlurRenderer(BufferedRenderer *r);
BufferedRenderer *createRecBlurRenderer(BufferedRenderer *r, uint x);


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
	c->setRotation(Quaternion<>::fromEuler(Vec3(20, 0, 45)));
	scene.insert(c);

	Light *l = new Light();
	l->setPosition(Vec3(10, 0, 5));
	scene.insert(l);

	BufferedRenderer *ri = 0;
	ri = new DeferredShadingRenderer(new GBufferRenderer(new SceneRenderer(&scene)));

	//FrameBufferRenderer renderer(createRecBlurRenderer(ri, 500));
	//FrameBufferRenderer *renderer[] = {new FrameBufferRenderer(createBlurRenderer(ri)), new FrameBufferRenderer(createUBlurRenderer(ri)), new FrameBufferRenderer(ri)};
	FrameBufferRenderer renderer(ri);


	while(run(win)) {

		c->setRotation(c->getRotation() * Quaternion<>::fromAxisAngle(Vec3(0, 0, 1), dMouse.x()) * Quaternion<>::fromAxisAngle(Vec3(0, 1, 0), dMouse.y()));

		(renderer)();

		GLContext::getContext()->finishTasks();
		GLContext::getContext()->flush();

		if(GLContext::getContext()->checkGLError()) {
			fatal("OpenGL error");
		}
	}

	return 0;
}
/*
BufferedRenderer *createRecBlurRenderer(BufferedRenderer *r, uint x) {
	return x ? createRecBlurRenderer(createRecBlurRenderer(r, x - 1)) : r;
}

BufferedRenderer *createRecBlurRenderer(BufferedRenderer *r) {
	return new ScreenShaderRenderer(createRecBlurShader(), r, "n_0");
}

BufferedRenderer *createHBlurRenderer(BufferedRenderer *r) {
	return new ScreenShaderRenderer(createBlurShader(Vec2(1, 0)), r, "n_0");
}

BufferedRenderer *createVBlurRenderer(BufferedRenderer *r) {
	return new ScreenShaderRenderer(createBlurShader(Vec2(0, 1)), r, "n_0");
}

BufferedRenderer *createBlurRenderer(BufferedRenderer *r) {
	return createVBlurRenderer(createHBlurRenderer(r));
}

BufferedRenderer *createUBlurRenderer(BufferedRenderer *r) {
	return new ScreenShaderRenderer(createUBlurShader(Vec2(0, 1)), new ScreenShaderRenderer(createUBlurShader(Vec2(1, 0)), r, "n_0"), "n_0");
}

ShaderCombinaison *createRecBlurShader() {
	static ShaderCombinaison *sh = 0;
	if(!sh) {
		sh = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"

			"in vec2 n_TexCoord;"
			"out vec4 color;"
			"void main() {"
				"vec2 offset = vec2(1.0) / vec2(800, 600);"
				"vec4 acc = vec4(0);"
				"for(int x = -1; x != 2; x++) {"
					"for(int y = -1; y != 2; y++) {"
						"acc += texture(n_0, n_TexCoord + vec2(x, y) * offset);"
					"}"
				"}"
				"color = acc / 9;"
			"}"), ShaderCombinaison::NoProjectionShader);
	}
	return sh;
}

ShaderCombinaison *createBlurShader(Vec2 dir) {
	dir.normalize();
	ShaderCombinaison *sh = new ShaderCombinaison(new Shader<FragmentShader>(
		"#version 420 core\n"
			"uniform sampler2D n_0;"

			"in vec2 n_TexCoord;"
			"out vec4 color;"

			"vec2 cl(vec2 x) { return clamp(x, vec2(0), vec2(1)); }"
			"float sqr(float x) { return x * x; }"
			"float iv(float x) { return 1.0 / (sqr(x * 0.1) * 10); }"

			"void main() {"
				"vec4 acc = vec4(0);"
				"float sum = 0;"
				"vec2 dir = vec2(" + String(dir.x()) + ", " + String(dir.y()) + ") / textureSize(n_0, 0);"
				"float size = 300;"
				"float isize = iv(size);"
				"float hsize = ceil(size / 2);"
				"float mi = exp(-sqr(hsize + 1) * isize);"
				"for(float x = -hsize; x < hsize; x++) {"
					"float w = exp(-sqr(x) * isize) - mi;"
					"acc += w * texture(n_0, cl(n_TexCoord + vec2(x) * dir));"
					"sum += w;"
				"}"
				"color = acc / sum;"
			"}"
		), ShaderCombinaison::NoProjectionShader);
	return sh;
}

ShaderCombinaison *createUBlurShader(Vec2 dir) {
	dir.normalize();
	ShaderCombinaison *sh = new ShaderCombinaison(new Shader<FragmentShader>(
		"#version 420 core\n"
			"uniform sampler2D n_0;"
			"uniform vec2 dir;"

			"in vec2 n_TexCoord;"
			"out vec4 color;"

			"vec2 cl(vec2 x) { return clamp(x, vec2(0), vec2(1)); }"
			"float sqr(float x) { return x * x; }"
			"float iv(float x) { return 1.0 / (sqr(x * 0.1) * 10); }"

			"void main() {"
				"vec4 acc = vec4(0);"
				"float sum = 0;"
				"float size = 300;"
				"float isize = iv(size);"
				"float hsize = ceil(size / 2);"
				"float mi = exp(-sqr(hsize + 1) * isize);"
				"for(float x = -hsize; x < hsize; x++) {"
					"float w = exp(-sqr(x) * isize) - mi;"
					"acc += w * texture(n_0, cl(n_TexCoord + vec2(x) * dir));"
					"sum += w;"
				"}"
				"color = acc / sum;"
			"}"
		), ShaderCombinaison::NoProjectionShader);
	(*sh)["dir"] = dir / Vec2(800, 600);
	return sh;
}

ShaderCombinaison *createNoiseShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
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

		shader = new ShaderCombinaison(frag, ShaderCombinaison::NoProjectionShader);
		std::cerr<<shader->getLogs()<<std::endl;
		(*shader)["scale"] = 2.464;
		(*shader)["d"] = 5;
		(*shader)["br"] = 0.02;
	}
	return shader;
}*/

/*ShaderCombinaison *createShader() {
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
			//"if(vertexFactor() < 0.1) {"
			//	"vec3 b = vec3((id >> 0) & 0xFF, (id >> 8) & 0xFF, (id >> 16) & 0xFF);"
			//	"color = vec4(vec3(b / vec3(0xwFF, 16, 0xFF)), 1.0);"
			//"}"
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
}*/

#else

#include <iostream>
#include <n/core/Timer.h>
#include <n/core/String.h>
#include <n/core/Array.h>
#include <n/core/Functor.h>
#include <n/math/Vec.h>
#include <n/mem/SmallObject.h>
#include <n/concurrent/SpinLock.h>
#include <n/concurrent/Async.h>
#include <n/concurrent/Mutex.h>


using namespace n;


int main(int, char **) {
	core::String str = "lol";
	std::cout<<(str < "lil")<<" "<<("lal" < str)<<std::endl;


}

#endif

