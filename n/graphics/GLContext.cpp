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
#include "ShaderProgram.h"
#include "FrameBufferPool.h"
#include "Material.h"
#include "VertexArrayFactory.h"
#include "VertexArrayObject.h"
#include "ShaderInstanceFactory.h"
#include <n/concurrent/Thread.h>
#include <n/core/Timer.h>


#define N_SYNCHRONOUS_GL_DEBUG

namespace n {
namespace graphics {

GLContext *GLContext::getContext() {
	static GLContext *ct = 0;
	if(!ct) {
		ct = new GLContext();
		ShaderProgram().bind();
		Material().bind(RenderFlag::ForceMaterialRebind);
	}
	return ct;
}

void GLContext::flush() {
	gl::flush();
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

GLContext::GLContext() : program(0), frameBuffer(0), fbPool(new FrameBufferPool()), vaoFactory(0), shFactory(new ShaderInstanceFactory()), viewport(1024, 768), screen(0) {
	if(concurrent::Thread::getCurrent()) {
		fatal("n::graphics::Context not created on main thread.");
	}

	if(!gl::initialize()) {
		fatal("Unable to create OpenGL context");
	}

	int major = gl::getInt(gl::MajorVersion);
	int minor = getInt(gl::MajorVersion);
	if(major < 4 || (major == 4 && minor < 3)) {
		fatal("This needs OpenGL 4.3 or newer to run.");
	}

	gl::setEnabled(gl::SeamlessCubeMaps);

	gl::setViewport(math::Vec2i(0), viewport);

	for(uint i = 0; i != Max; i++) {
		hwInts[i] = 0;
	}

	hwInts[MaxFboAttachements] = gl::getInt(gl::MaxFboAttachements);
	hwInts[MaxTextures] = gl::getInt(gl::MaxTextureUnits);
	hwInts[MaxVertexAttribs] = gl::getInt(gl::MaxVertexAttrib);
	hwInts[MaxVaryings] = gl::getInt(gl::MaxVaryingVectors);
	hwInts[MaxUBOBytes] = gl::getInt(gl::MaxUBOSize);
	hwInts[BindlessTextureSupport] = gl::isExtensionSupported("GL_ARB_bindless_texture");

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

	vaoFactory = new VertexArrayFactory<>();

	setDebugEnabled(true);

	#ifdef N_SYNCHRONOUS_GL_DEBUG
	gl::setEnabled(gl::SynchronousDebugging);
	#warning Synchronous debugging
	#endif
}

GLContext::~GLContext() {
	delete screen;
	finishTasks();
}

void GLContext::setDebugEnabled(bool deb) {
	gl::setEnabled(gl::Debug, deb);
}

void GLContext::auditGLState() {
	// ???
}

void GLContext::setViewport(const math::Vec2ui &v) {
	viewport = v;
	if(!frameBuffer) {
		gl::setViewport(math::Vec2i(0), v);
	}
}

math::Vec2ui GLContext::getViewport() const {
	return frameBuffer ? frameBuffer->getSize() : viewport;
}

void GLContext::fatalIfError() {
	if(gl::checkError()) {
		fatal("OpenGL error, exiting...");
	}
}

void GLContext::setModelMatrix(const math::Matrix4<> &m) {
	static UniformBuffer<math::Matrix4<>> buffer(1);
	buffer[0] = m;
	models = buffer;
}

void GLContext::setMatrixBuffer(const UniformBuffer<math::Matrix4<> > &buffer) {
	models = buffer;
}

void GLContext::setViewMatrix(const math::Matrix4<> &m) {
	if(view == m) {
		return;
	}
	view = m;
}

void GLContext::setProjectionMatrix(const math::Matrix4<> &m) {
	if(projection == m) {
		return;
	}
	projection = m;
}

const math::Matrix4<> &GLContext::getProjectionMatrix() const {
	return projection;
}

const math::Matrix4<> &GLContext::getViewMatrix() const {
	return view;
}

const VertexArrayObject<> &GLContext::getScreen() const {
	if(!screen) {
		screen = new VertexArrayObject<>((*vaoFactory)(TriangleBuffer<>::getScreen()));
	}
	return *screen;
}

ShaderProgram GLContext::getShaderProgram() const {
	return ShaderProgram(program);
}

VertexArrayFactory<> &GLContext::getVertexArrayFactory() {
	return *vaoFactory;
}

}
}




