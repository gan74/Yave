/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef MAIN
#define MAIN

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
#include <n/graphics/ShaderProgram.h>
#include <n/graphics/Camera.h>
#include <n/graphics/StaticMesh.h>
#include <n/graphics/Material.h>
#include <n/graphics/MeshLoader.h>
#include <n/graphics/MaterialLoader.h>
#include <n/graphics/SceneRenderer.h>
#include <n/graphics/FrameBufferRenderer.h>
#include <n/graphics/FrameBuffer.h>
#include <n/graphics/GBufferRenderer.h>
#include <n/graphics/DynamicBuffer.h>
#include <n/graphics/DeferredShadingRenderer.h>
#include <n/graphics/ScreenShaderRenderer.h>
#include <n/graphics/ShaderRenderer.h>
#include <n/graphics/TriangleBuffer.h>
#include <n/graphics/Light.h>


#include <n/math/StaticConvexVolume.h>
#include <n/math/ConvexVolume.h>

#include "Console.h"

Vec2 mouse;
Vec2 wasd;

class IThread : public n::concurrent::Thread
{
	public:
		IThread(uint *x) : n::concurrent::Thread(), i(x) {
		}

		virtual void run() override {
			while(true) {
				int w = 4;
				std::cin>>w;
				if(w < 0 || w > 2) {
					std::cerr<<"Invalid input"<<std::endl;
					continue;
				}
				*i = uint(w);
			}
		}

	private:
		uint *i;

};

SDL_Window *createWindow() {
	SDL_Window *mainWindow = 0;
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fatal("Unable to initialize SDL");
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
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
	static bool m1 = false;
	while(SDL_PollEvent(&e)) {
		if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
			cc = false;
			break;
		} else if(e.type == SDL_KEYDOWN && !e.key.repeat) {
			wasd += Vec2((e.key.keysym.sym == 'z') - (e.key.keysym.sym == 's'), (e.key.keysym.sym == 'q') - (e.key.keysym.sym == 'd'));
		} else if(e.type == SDL_KEYUP && !e.key.repeat) {
			wasd -= Vec2((e.key.keysym.sym == 'z') - (e.key.keysym.sym == 's'), (e.key.keysym.sym == 'q') - (e.key.keysym.sym == 'd'));
		} else if(e.type == SDL_MOUSEMOTION && m1) {
			mouse += Vec2(-e.motion.xrel, e.motion.yrel);
		} else if(e.type == SDL_MOUSEBUTTONDOWN) {
			m1 |= e.button.button == SDL_BUTTON_LEFT;
		} else if(e.type == SDL_MOUSEBUTTONUP) {
			m1 &= e.button.button != SDL_BUTTON_LEFT;
		}
	}
	return cc;
}

class Obj : public StaticMesh
{
	using Transformable<>::radius;
	public:
		Obj(String n) : StaticMesh(MeshLoader::load<String>(n)), model(n), autoScale(0) {
		}

		Obj(const MeshInstance<> &n) : StaticMesh(n), model(""), autoScale(0) {
		}

		void setAttribs(const VertexAttribs &a) {
			attribs = a;
		}

		virtual void render(RenderQueue &qu) override {
			if(!getMeshInstance().isValid()) {
				fatal("Unable to load mesh");
			}
			radius = getMeshInstance().getRadius();
			if(autoScale && !getMeshInstance().isNull()) {
				setScale(autoScale / getMeshInstance().getRadius());
			}
			for(MeshInstanceBase<> *b : getMeshInstance()) {
				qu.insert(RenderBatch(getTransform().getMatrix(), b, attribs));
			}
		}

		void setAutoScale(float s) {
			autoScale = s;
		}

	private:
		VertexAttribs attribs;
		String model;
		float autoScale;
};

class RandObj : public Obj
{
	public:
		RandObj(float x = 1000) : Obj("scube.obj") {
			setPosition(Vec3(random(), random(), random()) * 1000 - x / 2);
		}
};

/*ShaderCombinaison *createPerlinShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {


		shader = new ShaderCombinaison(frag, vert);
		std::cerr<<shader->getLogs()<<std::endl;
		(*shader)["scale"] = 2.464;
		(*shader)["d"] = 5;
		(*shader)["br"] = 0.02;
	}
	return shader;
}*/

Material<> createPerlinMat() {
		String str = "const float pi = " + String(pi<>()) + ";"
			"const float scale = 2.4;"
			"const int d = 5;"
			"const float br = 0.0;"
			"float rand(vec2 co) {"
				"return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);"
			"}"
			"vec2 grd(vec2 c) {"
				"float gx = rand(c);"
				"gx = gx * 2.0 * pi;"
				"return /*normalize*/(vec2(cos(gx), sin(gx)));"
			"}"
			"float dotGrd(vec2 p, ivec2 c) {"
				"return dot(grd(vec2(c)), p - vec2(c));"
			"}"
			"float smoothStep(float x) {"
				"return x * x * (3.0 - 2.0 * x);"
			"}"
			"vec2 fade(vec2 x) {"
				"return vec2(smoothStep(smoothStep(x.x)), smoothStep(smoothStep(x.y)));"
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
			"float pnoise(vec2 p) {"
				"float tot = 0;"
				"float n = 0;"
				"for(int i = 0; i < d; i++) {"
					"float w = pow(0.5, i);"
					"n += w * cnoise(p * (1 + scale) / w);"
					"tot += w;"
				"}"
				"n /= tot;"
				"float r = rand(p);"
				"return mix(n, r, br);"
			"}";

		Shader<FragmentShader> *frag = new Shader<FragmentShader>(str +
			"in vec2 tex;"
			"layout(location = 0) out vec4 n_0;"
			"layout(location = 1) out vec3 n_1;"
			"void main() {"
				"float h = pnoise(tex) * 100;"
				"vec2 n = vec2(dFdx(h), dFdy(h));"
				"n_0 = n_gbuffer0(vec4(1), vec3(0), 1, 0);"
				"n_1 = n_gbuffer1(vec4(1), vec3(n, sqrt(1.0 - dot(n, n))), 1, 0);"
			"}");

		Shader<VertexShader> *vert = new Shader<VertexShader>(str +
			"layout(location = 0) in vec3 n_VertexPosition;"
			"layout(location = 3) in vec2 n_VertexCoord;"
			"uniform mat4 n_ViewProjectionMatrix;"
			"uniform mat4 n_ModelMatrix;"
			"out vec2 tex;"
			"void main() {"
				"tex = n_VertexCoord;"
				"float height = pnoise(n_VertexCoord);"
				"float h = height * 10;"
				"vec4 model = n_ModelMatrix * vec4(n_VertexPosition.xy, h, 1.0);"
				"gl_Position = n_ViewProjectionMatrix * model;"
			"}");

		std::cout<<vert->getLogs()<<std::endl;
		std::cout<<frag->getLogs()<<std::endl;

	graphics::internal::Material<> mat;
	mat.prog = ShaderProgram(frag, vert);
	return Material<>(mat);
}


class PerlinTerrain : public StaticMesh
{
	public:
		PerlinTerrain() : StaticMesh(MeshInstance<>(std::move(TriangleBuffer<>::getGrid(100)), createPerlinMat())) {
		}

		virtual void render(RenderQueue &qu) override {
			for(MeshInstanceBase<> *b : getMeshInstance()) {
				qu.insert(RenderBatch(getTransform().getMatrix(), b, VertexAttribs()));
			}
		}
};


template<int I>
class Dummy
{
};

template<int I>
class DummyObj : public Dummy<I>, public DummyObj<I - 1>
{

	public:
		static void insert(uint max, Scene *s) {
			for(uint i = 0; i != max; i++) {
				s->insert(new DummyObj<I>());
			}
			DummyObj<I - 1>::insert(max, s);
		}
};

template<>
class DummyObj<0> : public Movable<>, public Renderable
{
	public:
		virtual void render(RenderQueue &) override {
		}

		static void insert(uint max, Scene *s) {
			for(uint i = 0; i != max; i++) {
				s->insert(new DummyObj<0>());
			}
		}

};

#endif // MAIN

