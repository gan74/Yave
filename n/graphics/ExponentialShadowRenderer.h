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

#ifndef N_GRAPHICS_EXPONENTIALSHADOWRENDERER_H
#define N_GRAPHICS_EXPONENTIALSHADOWRENDERER_H

#include "BufferedRenderer.h"
#include "ShadowRenderer.h"

namespace n {
namespace graphics {

class ExponentialShadowRenderer : public ShadowRenderer
{
	public:
		ExponentialShadowRenderer(ShadowRenderer *c, uint fHStep = 0, float exp = 30);

		~ExponentialShadowRenderer() {
			delete blurs[0];
			delete blurs[1];
		}

		virtual void *prepare() override {
			void *ptr = child->prepare();
			proj = child->getProjectionMatrix();
			view = child->getViewMatrix();
			return ptr;
		}

		virtual void render(void *ptr) override;

	private:
		ShadowRenderer *child;
		float exponent;
		ShaderInstance *blurs[2];
};


}
}

#endif // EXPONENTIALSHADOWRENDERER_H
