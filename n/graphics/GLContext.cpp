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
#include <n/concurent/Thread.h>
#include <n/core/Timer.h>

#include <iostream>

namespace n {
namespace graphics {

GLAPIENTRY void debugOut(gl::GLenum, gl::GLenum type, gl::GLuint, gl::GLuint sev, gl::GLsizei len, const char *msg, const void *) {
	if(sev == GL_DEBUG_SEVERITY_NOTIFICATION) {
		return;
	}
	core::String t;
	switch(type) {
		case GL_DEBUG_TYPE_PERFORMANCE:
			t = "perf";
		break;
		case GL_DEBUG_TYPE_PORTABILITY:
			t = "port";
		break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			t = "depr";
		break;
		default:
			t = "err";
		break;
	}

	std::cerr<<"[GL]"<<(sev == GL_DEBUG_SEVERITY_HIGH ? "[HIGH]" : "")<<"["<<t<<"] "<<core::String(msg, len)<<std::endl;
}

GLContext *GLContext::getContext() {
	static GLContext *ct = new GLContext();
	return ct;
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

GLContext::GLContext() {
	if(concurent::Thread::getCurrent()) {
		fatal("n::graphics::Context not created on main thread.");
	}
	gl::glewExperimental = true;
	if(gl::glewInit() != GLEW_OK) {
		fatal("Unable to initialize glew.");
	}
	std::cout<<"Running on "<<gl::glGetString(GL_VENDOR)<<" "<<gl::glGetString(GL_RENDERER)<<" using GL "<<gl::glGetString(GL_VERSION)<<std::endl;

	gl::glEnable(GL_TEXTURE_2D);
	gl::glEnable(GL_CULL_FACE);
	gl::glEnable(GL_DEPTH_TEST);

	gl::glEnable(GL_DEBUG_OUTPUT);
	gl::glDebugMessageCallback(&debugOut, 0);
	gl::glGetError();
}

GLContext::~GLContext() {
}

void GLContext::setDebugEnabled(bool deb) {
	if(deb) {
		gl::glEnable(GL_DEBUG_OUTPUT);
	} else {
		gl::glDisable(GL_DEBUG_OUTPUT);
	}
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
	shader->setValue("n_ModelMatrix", model);
	#endif
}

void GLContext::setViewMatrix(const math::Matrix4<> &m) {
	if(view == m) {
		return;
	}
	view = m;
	shader->setValue("n_ViewMatrix", m);
	shader->setValue("n_ViewProjectionMatrix", projection * m);
}

void GLContext::setProjectionMatrix(const math::Matrix4<> &m) {
	if(projection == m) {
		return;
	}
	projection = m;
	shader->setValue("n_ProjectionMatrix", m);
	shader->setValue("n_ViewProjectionMatrix", m * view);
}

}
}


