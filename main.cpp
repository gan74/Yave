#define ALL
#ifdef ALL
#include "main.h"

	Console console;

int main(int, char **) {
	SDL_Window *win = createWindow();

	Shader<VertexShader> vert("#version 420 core\n"
										"layout(location = 0) in vec3 n_VertexPosition;"
										"layout(location = 1) in vec3 n_VertexNormal;"
										"layout(location = 2) in vec3 n_VertexTangent;"
										"layout(location = 3) in vec2 n_VertexCoord;"
										"uniform mat4 n_ViewProjectionMatrix;"
										"uniform mat4 n_ModelMatrix;"
										"uniform vec3 n_Camera;"

										"smooth out vec3 N;"
										"out vec3 V;"
										"out vec3 T;"
										"out vec3 B;"
										"out vec2 U;"

										"void main() {"
											"vec4 model = n_ModelMatrix * vec4(n_VertexPosition, 1.0);"
											"gl_Position = n_ViewProjectionMatrix * model;"

											"V = normalize(n_Camera - model.xyz);"
											"N = mat3(n_ModelMatrix) * n_VertexNormal;"
											"T = mat3(n_ModelMatrix) * n_VertexTangent;"
											"U = n_VertexCoord;"
											"B = cross(N, T);"
										"}");

	io::File glsl("shader");
	if(!glsl.open(io::IODevice::Read)) {
		fatal("Unable to open shader");
	}
	char *brdfc = new char[glsl.size() + 1];
	brdfc[glsl.readBytes(brdfc)] = 0;
	String brdf(brdfc);
	delete[] brdfc;
	glsl.close();

	String fragStr = "#version 420 core\n"
					 "#define PI 3.14159265358979323846\n"
					 "layout(location = 0) out vec4 n_FragColor;"
					 "uniform vec4 n_Color;"
					 "uniform float n_Roughness;"
					 "uniform float n_Metallic;"
					 "uniform sampler2D n_Diffuse;"
					 "smooth in vec3 N;"
					 "in vec3 V;"
					 "in vec3 T;"
					 "in vec3 B;"
					 "in vec2 U;"

					 + brdf +

					"void main() {"
						"vec4 C = n_Color * texture(n_Diffuse, U);"
						"vec3 L = normalize(vec3(0.5, 0.5, 1.0));"
						"vec3 NN = normalize(N);"
						"n_FragColor = vec4((d_BRDF(C.rgb, L, V, NN) + s_BRDF(C.rgb, L, V, NN) * 0), C.a);"
					"}";

	Shader<FragmentShader> frag(fragStr);

	//GLContext::getContext()->setViewport(math::Vec2ui(1600, 900));


	ShaderCombinaison shader(&frag, &vert, 0);
	if(!shader.isValid()) {
		std::cerr<<shader.getLogs()<<std::endl;
		std::cerr<<frag.getLogs()<<std::endl;
		std::cerr<<vert.getLogs()<<std::endl;
		fatal("Unable to compile shader.");
	}

	shader.bind();

	FrameBuffer *buffer = 0;

	Camera cam;
	cam.setPosition(Vec3(0, 0, 5));
	cam.setRatio(4/3.0);
	shader["n_Camera"] = cam.getPosition();
	cam.setRotation(Quaternion<>::fromEuler(0, toRad(90), 0));

	GLContext::getContext()->setProjectionMatrix(cam.getProjectionMatrix());
	GLContext::getContext()->setViewMatrix(cam.getViewMatrix());
	Scene scene;

	scene.insert(&cam);

	/*auto c = new Obj("scube.obj");
	c->setPosition(Vec3(0, 2.5, 0));
	scene.insert(c);
	c = new Obj("sphere.obj");
	c->setPosition(Vec3(0, 0, 0));
	scene.insert(c);
	c = new Obj("scube.obj");
	c->setPosition(Vec3(0, -2.5, 0));
	scene.insert(c);*/

	auto c = new Obj("MP5_Scene.obj");
	c->setPosition(Vec3(0, 0, 0));
	c->setScale(10);
	scene.insert(c);


	uint num = 1;
	uint count = console("$objCount").to<uint>([&](){ num = 0; });
	for(uint i = 0; i < (count * num); i++) {
		scene.insert(new RandObj());
	}

	SceneRenderer renderer(&scene);

	console.start();

	while(run(win)) {
		Timer timer;
		uint size = uint(console("$size"));
		size = size ? size : 600;
		if(!buffer || (size && buffer->getSize().min() != size)) {
			delete buffer;
			math::Vec2ui vSize(size * Vec2(4, 3) / 3);
			std::cout<<"framebuffer size = "<<vSize<<std::endl;
			buffer = new FrameBuffer(vSize);
		}
		buffer->bind();

		gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderer();

		GLContext::getContext()->finishTasks();

		FrameBuffer::unbind();
		gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		buffer->blit();


		gl::glFlush();
		gl::glFinish();

		if(GLContext::getContext()->checkGLError()) {
			fatal("OpenGL error");
		}


		float fps = float(console("$framerate"));
		if(fps) {
			fps = (1.0 / fps);
			fps -= timer.elapsed();
			if(fps > 0.0) {
				concurent::Thread::sleep(fps);
			}
		}
	}
	return 0;
}


#else

#include <iostream>
#include <n/core/Timer.h>
#include <n/core/Array.h>
#include <n/core/Functor.h>
#include <n/mem/SmallObject.h>

using namespace n;
using namespace n::core;

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
	std::cout<<ShouldInsertAsCollection<core::Array<core::Functor<void()>>, core::Array<core::Functor<void()>>>::value<<std::endl;
}

#endif

