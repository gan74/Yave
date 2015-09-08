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

#include "VertexArrayObject.h"
#include "ShadowRenderer.h"
#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"
#include "Light.h"

namespace n {
namespace graphics {

void ShadowRenderer::poolBuffer() {
	if(buffer) {
		GLContext::getContext()->getFrameBufferPool().add(buffer);
		buffer = 0;
	}
}

void ShadowRenderer::createBuffer() {
	if(!buffer) {
		buffer = GLContext::getContext()->getFrameBufferPool().get(size, true, false);
	}
}

CameraShadowRenderer::CameraShadowRenderer(const Scene *sc, uint s) : ShadowRenderer(s), child(new SceneRenderer(sc)) {
}

CameraShadowRenderer::~CameraShadowRenderer() {
	delete child;
}

void *CameraShadowRenderer::prepare() {
	return child->prepare(createCamera());
}

void CameraShadowRenderer::render(void *ptr) {
	createBuffer();

	SceneRenderer::FrameData *sceneData = reinterpret_cast<SceneRenderer::FrameData *>(ptr);
	const Camera *cam = sceneData->camera;

	gl::glColorMask(false, false, false, false);
	gl::glEnable(GL_POLYGON_OFFSET_FILL);
	gl::glPolygonOffset(4.0, 1.0);

	buffer->bind();
	child->render(ptr, RenderFlag::FastDepth);

	gl::glColorMask(true, true, true, true);
	gl::glDisable(GL_POLYGON_OFFSET_FILL);

	delete cam;
}

Camera *BoxLightShadowRenderer::createCamera() {
	OrthographicCamera *cam = new OrthographicCamera(light->getSize());
	cam->setPosition(light->getPosition());
	cam->setRotation(light->getRotation());
	proj = cam->getProjectionMatrix();
	view = cam->getViewMatrix();
	return cam;
}


Camera *SpotLightShadowRenderer::createCamera() {
	PerspectiveCamera *cam = new PerspectiveCamera(light->getCutOff(), light->getRadius());
	cam->setPosition(light->getPosition());
	cam->setRotation(light->getRotation());
	proj = cam->getProjectionMatrix();
	view = cam->getViewMatrix();
	return cam;
}


}
}
