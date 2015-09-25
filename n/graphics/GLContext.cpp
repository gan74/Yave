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

#include "GLContext.h"
#include "ShaderCombinaison.h"
#include "GL.h"
#include "FrameBufferPool.h"
#include "Material.h"
#include "VertexArrayObject.h"
#include <n/concurrent/Thread.h>
#include <n/core/Timer.h>

#include <iostream>

#define GL_AUDIT_SATET

namespace n {
namespace graphics {

GLAPIENTRY void debugOut(gl::GLenum, gl::GLenum type, gl::GLuint, gl::GLuint sev, gl::GLsizei len, const char *msg, const void *) {
	if(sev == GL_DEBUG_SEVERITY_NOTIFICATION) {
		return;
	}
	if(type == GL_DEBUG_TYPE_PERFORMANCE) {
		return;
	}
	core::String t;
	switch(type) {
		case GL_DEBUG_TYPE_PERFORMANCE:
			t = "[perf]";
		break;
		case GL_DEBUG_TYPE_PORTABILITY:
			t = "[port]";
		break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			t = "[depr]";
		break;
		case GL_DEBUG_TYPE_ERROR:
			t = "[err]";
		break;
		default:
		break;
	}

	std::cerr<<std::endl<<"[GL]"<<(sev == GL_DEBUG_SEVERITY_HIGH ? "[HIGH]" : "")<<t<<" "<<core::String(msg, len)<<std::endl;
}

GLContext *GLContext::getContext() {
	static GLContext *ct = 0;
	if(!ct) {
		ct = new GLContext();
		ShaderProgram(ct->program).bind();
		Material().bind(RenderFlag::ForceMaterialRebind);
	}
	return ct;
}

void GLContext::flush() {
	gl::glFlush();
}

void GLContext::addGLTask(const core::Functor<void()> &f) {
	tasks.queue(f);
}

bool GLContext::processTasks() {
	core::Option<core::Functor<void()>> tsk = tasks.tryDequeue();
	if(tsk) {
		tsk.get()();
		return true;
	}
	return false;
}

void GLContext::finishTasks() {
	core::List<core::Functor<void()>> tsk = tasks.getAndClear();
	for(const core::Functor<void()> &f : tsk) {
		f();
	}
}

GLContext::GLContext() : shader(0), program(ShaderProgram::getNullProgram()), frameBuffer(0), fbPool(new FrameBufferPool()), viewport(1024, 768), screen(0) {
	if(concurrent::Thread::getCurrent()) {
		fatal("n::graphics::Context not created on main thread.");
	}
	gl::glewExperimental = true;
	if(gl::glewInit() != GLEW_OK) {
		fatal("Unable to initialize glew.");
	}
	std::cout<<"Running on "<<gl::glGetString(GL_VENDOR)<<" "<<gl::glGetString(GL_RENDERER)<<" using GL "<<gl::glGetString(GL_VERSION);
	#ifdef N_32BITS
	std::cout<<" (32 bits)";
	#endif
	std::cout<<std::endl;

	int major = 0;
	int minor = 0;
	gl::glGetIntegerv(GL_MAJOR_VERSION, &major);
	gl::glGetIntegerv(GL_MINOR_VERSION, &minor);
	if(major < 4 || (major == 4 && minor < 3)) {
		fatal("This needs OpenGL 4.3 or newer to run.");
	}

	//gl::glEnable(GL_TEXTURE_2D);
	//gl::glEnable(GL_CULL_FACE);
	//gl::glEnable(GL_DEPTH_TEST);
	gl::glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	gl::glViewport(0, 0, viewport.x(), viewport.y());

	for(uint i = 0; i != Max; i++) {
		hwInts[i] = 0;
	}

	gl::glGetIntegerv(GL_MAX_DRAW_BUFFERS, &hwInts[MaxFboAttachements]);
	gl::glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &hwInts[MaxTextures]);
	gl::glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &hwInts[MaxVertexAttribs]);

	if(hwInts[MaxVertexAttribs] <= 4) {
		fatal("Not enought vertex attribs.");
	}

	#define CHECK_TEX_FORMAT(frm) if(!Texture::isHWSupported(ImageFormat::frm)) { fatal(("Texture format " + core::String(#frm) + " not supported.").toChar()); }

	CHECK_TEX_FORMAT(RGBA32F);
	CHECK_TEX_FORMAT(RGBA16);
	CHECK_TEX_FORMAT(RG16);
	CHECK_TEX_FORMAT(F32);
	CHECK_TEX_FORMAT(Depth32);

	#undef CHECK_TEX_FORMAT

	gl::glGetError();

	gl::glDebugMessageCallback(&debugOut, 0);
	gl::glEnable(GL_DEBUG_OUTPUT);
}

GLContext::~GLContext() {
	delete screen;
	finishTasks();
}

void GLContext::setDebugEnabled(bool deb) {
	if(deb) {
		gl::glEnable(GL_DEBUG_OUTPUT);
	} else {
		gl::glDisable(GL_DEBUG_OUTPUT);
	}
}

void GLContext::auditGLState() {
	#ifdef GL_AUDIT_SATET
	#warning GL state auditing enabled
	int handle = 0;
	gl::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &handle);
	if(handle != int(frameBuffer ? frameBuffer->handle : 0)) {
		fatal("Invalid GL framebuffer state.");
	}
	#endif
}

void GLContext::setViewport(const math::Vec2ui &v) {
	viewport = v;
	if(!frameBuffer) {
		gl::glViewport(0, 0, viewport.x(), viewport.y());
	}
	shader->setValue("n_ViewportSize", math::Vec2(viewport));
}

math::Vec2ui GLContext::getViewport() const {
	return frameBuffer ? frameBuffer->getSize() : viewport;
}

bool GLContext::checkGLError() {
	int error = gl::glGetError();
	if(error) {
		fatal(("Warning : error : " + (error == GL_INVALID_OPERATION ? core::String("INVALID OPERATION") :
			(error == GL_INVALID_ENUM ? core::String("INVALID ENUM") :
			(error == GL_INVALID_VALUE ? core::String("INVALID VALUE") :
			(error == GL_OUT_OF_MEMORY ? core::String("OUT OF MEMORY") : core::String(error)))))).toChar());
		return true; // just in case...
	}
	return false;
}

void GLContext::setModelMatrix(const math::Matrix4<> &m) {
	model = m;
	#ifdef N_USE_MATRIX_BUFFER
	matrixBuffer->set(model, 0);
	matrixBuffer->bind(0);
	#else
	if(shader) {
		shader->setValue("n_ModelMatrix", model);
	}
	#endif
}

void GLContext::setViewMatrix(const math::Matrix4<> &m) {
	if(view == m) {
		return;
	}
	view = m;
	if(shader) {
		shader->setValue("n_ViewMatrix", m);
		shader->setValue("n_ViewProjectionMatrix", projection * m);
	}
}

void GLContext::setProjectionMatrix(const math::Matrix4<> &m) {
	if(projection == m) {
		return;
	}
	projection = m;
	if(shader) {
		shader->setValue("n_ProjectionMatrix", m);
		shader->setValue("n_ViewProjectionMatrix", m * view);
	}
}

const math::Matrix4<> &GLContext::getProjectionMatrix() const {
	return projection;
}

const math::Matrix4<> &GLContext::getViewMatrix() const {
	return view;
}

const math::Matrix4<> &GLContext::getModelMatrix() const {
	return model;
}

const VertexArrayObject<float> &GLContext::getScreen() const {
	if(!screen) {
		screen = new VertexArrayObject<float>(TriangleBuffer<>::getScreen());
	}
	return *screen;
}

}
}




