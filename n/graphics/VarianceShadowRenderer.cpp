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

#include "VarianceShadowRenderer.h"
#include "FrameBufferPool.h"

namespace n {
namespace graphics {

ShaderCombinaison *getVSMShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		shader = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"

			"in vec2 n_TexCoord;"
			"out vec2 n_Out;"

			"void main() {"
				"float depth = texture(n_0, n_TexCoord).x * 0.5 + 0.5;"
				"float dx = dFdx(depth);"
				"float dy = dFdy(depth);"
				"n_Out = vec2(depth, depth * depth + 0.25 * (dx * dx + dy * dy));"
			"}"), ShaderProgram::NoProjectionShader);
	}
	return shader;
}

VarianceShadowRenderer::VarianceShadowRenderer(ShadowRenderer *c, uint fHStep) : ShadowRenderer(c->getSize().x(), false, ImageFormat::RG32F), child(c), blurs{createBlurShader(false, fHStep ? fHStep : core::log2ui(c->getSize().x())), createBlurShader(true, fHStep ? fHStep : core::log2ui(c->getSize().x()))} {
	mapIndex = 0;
	shaderCode = "vec3 proj = projectShadow(pos);"
				 "float distance = (proj.z * 0.5 + 0.5);"
				 "vec2 moments = texture(n_LightShadow, proj.xy).xy;"
				 //"moments.x = max(moments.x, max(max(textureOffset(n_LightShadow, proj.xy, ivec2(1, 0)).x, textureOffset(n_LightShadow, proj.xy, ivec2(-1, 0)).x), max(textureOffset(n_LightShadow, proj.xy, ivec2(0, 1)).x, textureOffset(n_LightShadow, proj.xy, ivec2(0, -1)).x)));"
				 "if(distance <= moments.x) {"
					 "return 1.0;"
				 "}"
				 "float variance = moments.y - sqr(moments.x);"
				 "variance = max(variance, 0.00002);"
				 "float p_max = variance / (variance + sqr(distance - moments.x));"
				 "return clamp(p_max, 0.0, 1.0);";
}

void VarianceShadowRenderer::render(void *ptr) {
	child->render(ptr);

	getFrameBuffer().bind();
	ShaderCombinaison *sh = getVSMShader();
	sh->setValue(SVTexture0, child->getShadowMap());
	sh->bind();
	GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

	FrameBuffer *temp = GLContext::getContext()->getFrameBufferPool().get(getSize(), false, ImageFormat::RG32F);
	blurs[1]->setValue(SVTexture0, temp->getAttachement(0));
	blurs[0]->setValue(SVTexture0, getFrameBuffer().getAttachement(0));

	temp->bind();
	blurs[0]->bind();
	GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

	getFrameBuffer().bind();
	blurs[1]->bind();
	GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

	blurs[1]->unbind();
	GLContext::getContext()->getFrameBufferPool().add(temp);
	child->discardBuffer();
}

}
}
