#define ALL
#ifdef ALL
#include <iostream>

#include <n/core/Timer.h>

#include <n/math/Plane.h>
#include <n/io/File.h>

#include <SDL2/SDL.h>
#include <n/graphics/ImageLoader.h>
#include <n/graphics/gl/Context.h>
#include <n/graphics/gl/Texture.h>
#include <n/graphics/gl/Scene.h>
#include <n/graphics/gl/GL.h>
#include <n/graphics/gl/StaticBuffer.h>
#include <n/graphics/gl/TriangleBuffer.h>
#include <n/graphics/gl/VertexArrayObject.h>
#include <n/graphics/gl/ShaderCombinaison.h>
#include <n/graphics/gl/Camera.h>
#include <n/graphics/gl/StaticMesh.h>
#include <n/graphics/gl/Material.h>
#include <n/graphics/gl/MeshLoader.h>

#include <n/math/StaticConvexVolume.h>
#include <n/math/ConvexVolume.h>

using namespace n;
using namespace n::graphics;
using namespace n::math;
using namespace n::core;

SDL_Window *createWindow() {
	SDL_Window *mainWindow = 0;
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fatal("Unable to initialize SDL");
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	if(!(mainWindow = SDL_CreateWindow("n 2.1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN))) {
		fatal("Unable to create window");
	}
	SDL_GL_CreateContext(mainWindow);
	SDL_GL_SetSwapInterval(0);

	gl::Context::getContext();

	return mainWindow;
}

bool run(SDL_Window *mainWindow) {
	SDL_GL_SwapWindow(mainWindow);
	SDL_Event e;
	bool cc = true;
	while(SDL_PollEvent(&e)) {
		if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
			cc = false;
			break;
		}
	}
	return cc;
}

class Cube : public gl::StaticMesh
{
	public:
		Cube() : gl::StaticMesh(gl::MeshLoader::load<String>("scube.obj")) {
			axis = ((Vec3(random(), random(), random()) - 0.5) * 2).normalized();
		}

		void update() {
			static Timer timer;
			static double x = 0;
			double t = timer.reset();
			x += t;
			Quaternion<> q = Quaternion<>::fromAxisAngle(axis.cross(Vec3(1, 0, 0)).cross(axis), t);
			axis = q(axis);
			setRotation(Quaternion<>::fromAxisAngle(axis, x));
		}

	private:
		Vec3 axis;
};


int main(int, char **) {

	SDL_Window *win = createWindow();

	gl::Shader<gl::VertexShader> vert("#version 420 core\n"
										"layout(location = 0) in vec3 n_VertexPosition;"
										"layout(location = 1) in vec3 n_VertexNormal;"
										"uniform mat4 n_ViewProjectionMatrix;"
										"uniform mat4 n_ModelMatrix;"
										"out vec3 normal;"
										"out vec3 color;"
										"void main() {"
											"gl_Position = n_ViewProjectionMatrix * n_ModelMatrix * vec4(n_VertexPosition, 1.0);"
											"normal = (n_ModelMatrix * vec4(n_VertexNormal, 1.0)).xyz;"
											"color = n_VertexNormal;"
										"}");

	gl::Shader<gl::FragmentShader> frag("#version 420 core\n"
										"layout(location = 0) out vec4 n_FragColor;"
										"in vec3 normal;"
										"in vec3 color;"
										"void main() {"
											//"n_FragColor = vec4(color * 0.5 + 0.5, 1.0);"
											"n_FragColor = vec4(vec3(dot(normal, normalize(vec3(1, 1, 1))) * 0.75 + 0.25), 1.0);"
										"}");

	gl::Material<> mat;
	mat.setColor(Vec4(1, 0, 0, 1));


	gl::ShaderCombinaison shader(&frag, &vert, 0);
	if(!shader.isValid()) {
		std::cerr<<shader.getLogs()<<std::endl;
		std::cerr<<frag.getLogs()<<std::endl;
		std::cerr<<vert.getLogs()<<std::endl;
		fatal("Unable to compile shader.");
	}

	shader.bind();

	gl::Camera cam;
	cam.setPosition(Vec3(0, 0, 3));
	cam.setRotation(Quaternion<>::fromEuler(0, toRad(90), 0));

	gl::Context::getContext()->setProjectionMatrix(cam.getProjectionMatrix());
	gl::Context::getContext()->setViewMatrix(cam.getViewMatrix());
	/*Transform<> tr;
	Vec3 axis(random(), random(), random());*/

	gl::Scene scene;
	/*for(uint i = 0; i != 1000; i++) {
		Cube *cube = new Cube();
		cube->setPosition(Vec3(random(), random(), random()) * 100 - 50);
		scene.insert(cube);
	}*/

	scene.insert(new Cube());

	while(run(win)) {

		glClear(GL_COLOR_BUFFER_BIT);


		Timer timer;
		Array<gl::StaticMesh *> meshes = scene.query<gl::StaticMesh>(cam);
		for(auto x : scene.get<Cube>()) {
			x->update();
		}
		if(timer.elapsed() > 0.002) {
			std::cout<<meshes.size()<<" in "<<timer.elapsed() * 1000<<"ms"<<std::endl;
		}
		gl::RenderQueue queue;
		for(gl::Renderable *m : meshes) {
			m->render(queue);
		}
		for(const auto &q : queue.getBatches()) {
			q();
		}

		gl::Context::getContext()->processTasks();
		glFlush();

		if(gl::Context::checkGLError()) {
			fatal("GL error");
		}
	}
	return 0;
}


#else

#include <n/core/Array.h>

int main(int, char **) {
	return 0;
}

#endif

