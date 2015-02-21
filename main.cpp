#define ALL
#ifdef ALL
#include <iostream>

#include <n/core/Timer.h>

#include <n/math/Plane.h>
#include <n/io/File.h>

#include <SDL2/SDL.h>
#include <n/graphics/ImageLoader.h>
#include <n/graphics/GLContext.h>
#include <n/graphics/Texture.h>
#include <n/graphics/Scene.h>
#include <n/graphics/GL.h>
#include <n/graphics/StaticBuffer.h>
#include <n/graphics/TriangleBuffer.h>
#include <n/graphics/VertexArrayObject.h>
#include <n/graphics/ShaderCombinaison.h>
#include <n/graphics/Camera.h>
#include <n/graphics/StaticMesh.h>
#include <n/graphics/Material.h>
#include <n/graphics/MeshLoader.h>
#include <n/graphics/MaterialLoader.h>

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

	GLContext::getContext();

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

class Obj : public StaticMesh
{
	public:
		Obj(String n) : StaticMesh(MeshLoader::load<String>(n)) {
			axis = ((Vec3(random(), random(), random()) - 0.5) * 2).normalized();
		}

		virtual void render(RenderQueue &qu) override {
			static Timer timer;
			static double x = 0;
			double t = timer.reset();
			x += t;
			Quaternion<> q = Quaternion<>::fromAxisAngle(axis.cross(Vec3(1, 0, 0)).cross(axis), t);
			axis = q(axis);
			setRotation(Quaternion<>::fromAxisAngle(axis, x));
			if(!getMeshInstance().isValid()) {
				fatal("Unable to load mesh");
			}
			StaticMesh::render(qu);
		}

	private:
		Vec3 axis;
};

int main(int, char **) {

	SDL_Window *win = createWindow();

	Shader<VertexShader> vert("#version 420 core\n"
										"layout(location = 0) in vec3 n_VertexPosition;"
										"layout(location = 1) in vec3 n_VertexNormal;"
										"uniform mat4 n_ViewProjectionMatrix;"
										"uniform mat4 n_ModelMatrix;"
										"uniform vec3 n_Camera;"

										"out vec3 normal;"
										"out vec3 view;"

										"void main() {"
											"vec4 model = n_ModelMatrix * vec4(n_VertexPosition, 1.0);"
											"gl_Position = n_ViewProjectionMatrix * model;"
											"view = normalize(n_Camera - model.xyz);"
											"normal = mat3(n_ModelMatrix) * n_VertexNormal;"
										"}");

	Shader<FragmentShader> frag("#version 420 core\n"
										"#define PI 3.14159\n"
										"layout(location = 0) out vec4 n_FragColor;"
										"in vec3 normal;"
										"in vec3 view;"
										"uniform float n_F0;"
										"uniform vec4 n_Color;"
										"uniform float n_Metallic;"
										"uniform float n_Roughness;"

										"float saturate(float x) {"
											"return min(max(x, 0.0), 1.0);"
										"}"

										"float GGX(vec3 N, vec3 H, float R) {"
											"float NdotH = dot(N, H);"
											"float R2 = max(R * R, 0.0005);"
											"float NdotH2 = NdotH * NdotH;"
											"float D = NdotH2 * R2 + (1.0 - NdotH2);"
											"return R2 / (PI * D * D);"
										"}"

										"float G1(vec3 V, vec3 N, float R) {"
											/*"float K = R + 1.0;"
											"K = K * K / 8.0;"*/
											"float K = (R * R) * 0.5;"
											"float NdotV = max(0.0, dot(N, V));"
											"return NdotV / (NdotV * (1.0 - K) + K);"
										"}"

										"float GGGX(vec3 L, vec3 V, vec3 N, float R) {"
											"return G1(L, N, R) * G1(V, N, R);"
										"}"

										"vec3 F(float cosT, vec3 C, float M) {"
											"vec3 F0 = vec3(n_F0) * (1.0 - M) + C * M;"
											"return F0 + (1.0 - F0) * pow(1.0 - cosT, 5.0);"
										"}"

										"float ON(vec3 L, vec3 V, vec3 N, float R) {"
											"float R2 = R * R;"
											"float A = 1.0 - 0.5 * R2 / (R2 + 0.33);"
											"float B = 0.45 * R2 / (R2 + 0.09);"
											"float LdotV = dot(L, V);"
											"float NdotL = dot(L, N);"
											"float NdotV = dot(N, V);"
											"float s = LdotV - NdotL * NdotV;"
											"float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));"
											"float d = max(0.0, dot(normalize(V - N * NdotV), normalize(L - N * NdotL)));"
											"return max(0.0, NdotL) * (A + B * s / t);"
										"}"

										"float lambert(vec3 L, vec3 V, vec3 N, float R) {"
											"return dot(L, N);"
										"}"

										"void main() {"
											"vec3 L = normalize(vec3(0.5, 0.5, 1.0));"
											"vec3 H = normalize(view + L);"
											"vec3 S = GGGX(L, view, normal, n_Roughness) * GGX(normal, H, n_Roughness) * F(dot(L, H), n_Color.rgb, n_Metallic);"
											"float IS = 1.0 / max(1.0, 4.0 * dot(normal, L) * dot(normal, view));"
											"S = S * IS;"
											"n_FragColor = vec4(n_Color.rgb * (ON(L, view, normal, n_Roughness)) + S, n_Color.a);"
											//"n_FragColor = vec4(S, 1.0);"
										"}");

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
	/*for(uint i = 0; i != 1000; i++) {
		Cube *cube = new Cube();
		cube->setPosition(Vec3(random(), random(), random()) * 100 - 50);
		scene.insert(cube);
	}*/

	auto c = new Obj("sphere.obj");
	c->setPosition(Vec3(0, 2.5, 0));
	scene.insert(c);
	c = new Obj("cube.obj");
	c->setPosition(Vec3(0, 0, 0));
	scene.insert(c);
	c = new Obj("sphere.obj");
	c->setPosition(Vec3(0, -2.5, 0));
	scene.insert(c);


	shader["n_F0"] = 0.34;

	Timer timer;
	while(run(win)) {
		gl::glClear(GL_COLOR_BUFFER_BIT);
		core::Array<StaticMesh *> meshes = scene.query<StaticMesh>(cam);
		RenderQueue queue;
		for(Renderable *m : meshes) {
			m->render(queue);
		}
		shader["n_Metallic"] = sin(timer.elapsed()) * 0.5 + 0.5;
		int r = 0;
		for(const auto &q : queue.getBatches()) {

			shader["n_Roughness"] = r * 0.5;
			r++;
			q();

		}
		GLContext::getContext()->processTasks();
		if(GLContext::checkGLError()) {
			fatal("GL error");
		}
		//shader["n_Roughness"] = (1 + sin(timer.elapsed()));
		gl::glFlush();
	}
	return 0;
}


#else

#include <n/core/Array.h>

int main(int, char **) {
	return 0;
}

#endif

