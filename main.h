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

#include <windows.h>

#include <n/core/Timer.h>

#include <n/math/Plane.h>
#include <n/io/File.h>
#include <n/core/Lazy.h>
#include <n/core/AutoPtr.h>

#include <SDL2/SDL.h>
#include <n/graphics/ImageLoader.h>
#include <n/graphics/GLContext.h>
#include <n/graphics/Texture.h>
#include <n/graphics/Scene.h>
#include <n/graphics/GL.h>
#include <n/graphics/StaticBuffer.h>
#include <n/graphics/TriangleBuffer.h>
#include <n/graphics/VertexArrayObject.h>
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
#include <n/graphics/TriangleBuffer.h>
#include <n/graphics/Light.h>
#include <n/graphics/CubeMap.h>
#include <n/graphics/TextureBinding.h>
#include <n/math/StaticConvexVolume.h>
#include <n/math/ConvexVolume.h>
#include <n/math/Quaternion.h>
#include <n/graphics/BasicToneMapRenderer.h>
#include <n/graphics/OrthographicCamera.h>
#include <n/graphics/PerspectiveCamera.h>
#include <n/graphics/BufferRenderer.h>
#include <n/graphics/VarianceShadowRenderer.h>
#include <n/graphics/DeferredIBLRenderer.h>
#include <n/graphics/CubeFrameBuffer.h>


using namespace n;
using namespace n::core;
using namespace n::math;
using namespace n::graphics;

#include <iostream>

Vec2 mouse;
Vec2 wasd;
BasicToneMapRenderer *tone;
bool bench = false;
uint rendererIndex = 0;

SDL_Window *createWindow(Vec2ui winS = Vec2ui(1600, 900)) {
	SetConsoleOutputCP(65001);
	SDL_Window *mainWindow = 0;
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fatal("Unable to initialize SDL");
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	if(!(mainWindow = SDL_CreateWindow("n 2.1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winS.x(), winS.y(), SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN))) {
		fatal("Unable to create window");
	}
	SDL_GL_CreateContext(mainWindow);
	SDL_GL_SetSwapInterval(0);

	GLContext::getContext()->setViewport(winS);

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
			if(tone) {
				if(e.key.keysym.sym == 'u') { tone->setExposure(tone->getExposure() + 0.05); std::cout<<"exp = "<<tone->getExposure()<<std::endl; }
				if(e.key.keysym.sym == 'j') { tone->setExposure(tone->getExposure() - 0.05); std::cout<<"exp = "<<tone->getExposure()<<std::endl; }

				if(e.key.keysym.sym == 'h') { tone->setWhiteThreshold(tone->getWhiteThreshold() + 0.1); std::cout<<"wht = "<<tone->getWhiteThreshold()<<std::endl; }
				if(e.key.keysym.sym == 'k') { tone->setWhiteThreshold(tone->getWhiteThreshold() - 0.1); std::cout<<"wht = "<<tone->getWhiteThreshold()<<std::endl; }
			}
			if(e.key.keysym.sym >= '1' && e.key.keysym.sym <= '9') {
				rendererIndex = e.key.keysym.sym - '1';
			}
		} else if(e.type == SDL_KEYUP && !e.key.repeat) {
			wasd -= Vec2((e.key.keysym.sym == 'z') - (e.key.keysym.sym == 's'), (e.key.keysym.sym == 'q') - (e.key.keysym.sym == 'd'));
			if(tone && e.key.keysym.sym == 'e') {
				tone->setDebugEnabled(!tone->isDebugEnabled());
			}
			bench |= e.key.keysym.sym == 'b';
		} else if(e.type == SDL_MOUSEMOTION && m1) {
			mouse += Vec2(e.motion.xrel, e.motion.yrel);
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
	public:
		Obj(String n) : StaticMesh(MeshLoader::load<String>(n)), model(n), autoScale(0) {
		}

		Obj(const MeshInstance &n) : StaticMesh(n), model(""), autoScale(0) {
		}

		void setAttribs(const VertexAttribs &a) {
			attribs = a;
		}

		virtual void render(RenderQueue &qu, RenderFlag rf) override {
			if(!getMeshInstance().isValid()) {
				fatal("Unable to load mesh");
			}
			radius = getMeshInstance().getRadius();
			if(autoScale && !getMeshInstance().isNull()) {
				setScale(autoScale / getMeshInstance().getRadius());
			}
			for(SubMeshInstance *b : getMeshInstance()) {
				qu.insert(RenderBatch(getTransform().getMatrix(), b, attribs, rf));
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

class MaterialTest : public Movable, public Renderable
{
	public:
		MaterialTest(float m, float r) : Movable(), Renderable(), inst(0) {
			static VertexArrayObject<> vao = GLContext::getContext()->getVertexArrayFactory()(TriangleBuffer<>::getSphere());
			MaterialData data;
			data.surface.color = Vec4(0.5);
			data.surface.metallic = m;
			data.surface.roughness = Texture(Image(Vec2ui(1), ImageFormat::F32, &r));
			inst = new SubMeshInstance(vao, Material(data));
			radius = vao.getRadius();
		}

		virtual void render(RenderQueue &qu, RenderFlag rf) override {
			qu.insert(RenderBatch(transform.getMatrix(), inst, VertexAttribs(), rf));
		}

	private:
		SubMeshInstance *inst;
};

class DummyRenderable : public Movable, public Renderable
{
	public:
		DummyRenderable() : Movable(), Renderable() {
			radius = 1.0;
			setPosition(Vec3(random(), random(), random()) * 800 - 400);
		}

		virtual void render(RenderQueue &, RenderFlag) override {
		}
};

class RandObj : public Obj
{
	public:
		RandObj(float x = 1000) : Obj("scube.obj") {
			setPosition(Vec3(random(), random(), random()) * 1000 - x / 2);
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
class DummyObj<0> : public Movable, public Renderable
{
	public:
		virtual void render(RenderQueue &, RenderFlag) override {
		}

		static void insert(uint max, Scene *s) {
			for(uint i = 0; i != max; i++) {
				s->insert(new DummyObj<0>());
			}
		}

};

#endif // MAIN

