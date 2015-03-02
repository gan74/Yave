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
										"uniform mat4 n_ViewProjectionMatrix;"
										"uniform mat4 n_ModelMatrix;"
										"uniform vec3 n_Camera;"

										"out vec3 N;"
										"out vec3 V;"
										"out vec3 T;"
										"out vec3 B;"

										"void main() {"
											"vec4 model = n_ModelMatrix * vec4(n_VertexPosition, 1.0);"
											"gl_Position = n_ViewProjectionMatrix * model;"

											"V = normalize(n_Camera - model.xyz);"
											"N = mat3(n_ModelMatrix) * n_VertexNormal;"
											"T = mat3(n_ModelMatrix) * n_VertexTangent;"
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
					 "in vec3 N;"
					 "in vec3 V;"
					 "in vec3 T;"
					 "in vec3 B;"

					 + brdf +

					"void main() {"
						"vec3 C = n_Color.rgb;"
						"vec3 L = normalize(vec3(0.5, 0.5, 1.0));"
						"vec3 NN = normalize(N);"
						"n_FragColor = vec4((d_BRDF(C, L, V, NN) + s_BRDF(C, L, V, NN)), n_Color.a);"
					"}";

	Shader<FragmentShader> frag(fragStr);


	ShaderCombinaison shader(&frag, &vert, 0);
	if(!shader.isValid()) {
		std::cerr<<shader.getLogs()<<std::endl;
		std::cerr<<frag.getLogs()<<std::endl;
		std::cerr<<vert.getLogs()<<std::endl;
		fatal("Unable to compile shader.");
	}

	shader.bind();

	Camera cam;
	cam.setPosition(Vec3(0, 0, 5));
	shader["n_Camera"] = cam.getPosition();
	cam.setRotation(Quaternion<>::fromEuler(0, toRad(90), 0));

	GLContext::getContext()->setProjectionMatrix(cam.getProjectionMatrix());
	GLContext::getContext()->setViewMatrix(cam.getViewMatrix());
	Scene scene;

	scene.insert(&cam);

	auto c = new Obj("scube.obj");
	c->setPosition(Vec3(0, 2.5, 0));
	scene.insert(c);
	c = new Obj("sphere.obj");
	c->setPosition(Vec3(0, 0, 0));
	scene.insert(c);
	c = new Obj("scube.obj");
	c->setPosition(Vec3(0, -2.5, 0));
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
		gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderer();
		GLContext::getContext()->processTasks();
		if(GLContext::checkGLError()) {
			fatal("GL error");
		}
		gl::glFlush();
		gl::glFinish();

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

#include <n/core/Array.h>

int main(int, char **) {
	return 0;
}

#endif

