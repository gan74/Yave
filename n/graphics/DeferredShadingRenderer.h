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

#ifndef N_GRAPHICS_DEFERREDSHADINGRENDERER
#define N_GRAPHICS_DEFERREDSHADINGRENDERER

#include "GBufferRenderer.h"

namespace n {
namespace graphics {


class DeferredShadingRenderer : public BufferedRenderer
{
	struct FrameData;
	public:
		enum LightingDebugMode
		{
			None,
			Attenuation,
			Shadows,

			Max
		};

		enum ShadowOptimisationMode
		{
			Memory,
			DrawCalls
		};

		DeferredShadingRenderer(GBufferRenderer *c, const math::Vec2ui &s = math::Vec2ui(0));

		virtual ~DeferredShadingRenderer() {
		}

		virtual void *prepare() override;
		virtual void render(void *ptr) override;

		void setDebugMode(LightingDebugMode m) {
			debugMode = m;
		}

	private:
		template<typename T>
		friend ShaderCombinaison *lightPass(const FrameData *data, DeferredShadingRenderer *renderer);

		GBufferRenderer *child;
		LightingDebugMode debugMode;
		ShadowOptimisationMode shadowMode;
};

}
}


#endif // N_GRAPHICS_DEFERREDSHADINGRENDERER

