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

#include <n/defines.h>
#ifndef N_NO_GL

#include "Context.h"
#include "ShaderCombinaison.h"
#include "GL.h"
#include <n/concurent/Thread.h>
#include <n/core/Timer.h>

#include <iostream>

namespace n {
namespace graphics {
namespace gl {

Context *Context::getContext() {
	static Context *ct = new Context();
	return ct;
}

void Context::addGLTask(const core::Functor<void()> &f) {
	tasks.queue(f);
}

bool Context::processTasks() {
	core::Option<core::Functor<void()>> tsk = tasks.tryDequeue();
	if(tsk) {
		tsk.get()();
		return true;
	}
	return false;
}

Context::Context() {
	if(concurent::Thread::getCurrent()) {
		fatal("n::graphics::gl::Context not created on main thread.");
	}
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fatal("Unable to initialize glew.");
	}
	std::cout<<"Running on "<<glGetString(GL_VENDOR)<<" "<<glGetString(GL_RENDERER)<<" using GL "<<glGetString(GL_VERSION)<<std::endl;

	glEnable(GL_TEXTURE_2D);
	glGetError();
}

Context::~Context() {
}


bool Context::checkGLError() {
	int error = glGetError();
	if(error) {
		fatal(("Warning : error : " + (error == GL_INVALID_OPERATION ? core::String("INVALID OPERATION") :
			(error == GL_INVALID_ENUM ? core::String("INVALID ENUM") :
			(error == GL_INVALID_VALUE ? core::String("INVALID VALUE") :
			(error == GL_OUT_OF_MEMORY ? core::String("OUT OF MEMORY") : core::String(error)))))).toChar());
		return true; // just in case...
	}
	return false;
}

void Context::setModelMatrix(const math::Matrix4<> &m) {
	model = m;
	#ifdef N_USE_MATRIX_BUFFER
	matrixBuffer->set(model, 0);
	matrixBuffer->bind(0);
	#else
	shader->setValue("n_ModelMatrix", model);
	#endif
}

void Context::setViewMatrix(const math::Matrix4<> &m) {
	if(view == m) {
		return;
	}
	view = m;
	shader->setValue("n_ViewMatrix", m);
	shader->setValue("n_ViewProjectionMatrix", projection * m);
}

void Context::setProjectionMatrix(const math::Matrix4<> &m) {
	if(projection == m) {
		return;
	}
	projection = m;
	shader->setValue("n_ProjectionMatrix", m);
	shader->setValue("n_ViewProjectionMatrix", m * view);
}

}
}
}

#endif

