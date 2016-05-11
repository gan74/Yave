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

#ifndef N_GRAPHICS_IBLPROBE_H
#define N_GRAPHICS_IBLPROBE_H

#include "CubeMap.h"

namespace n {
namespace graphics {

class IBLProbe : NonCopyable
{
	public:
		struct BufferData
		{
			uint64 cube;
			float roughnessPower;
			int levels;
			int padding[2];
		};

		IBLProbe(const CubeMap &env);

		CubeMap getCubeMap();

		uint getLevelCount() const;
		float getRoughnessPower() const;

		float remapRoughness(float r) const;

		BufferData toBufferData();

		static core::String toShader() {
			return
				"struct n_IBLProbe {"
					"samplerCube cube;"

					"float roughnessPower;"
					"int levels;"
					"int padding[2];"
				"};"
				"vec4 iblProbe(n_IBLProbe p, vec3 d, float r) {"
					"r = pow(r, p.roughnessPower);"
					"return textureLod(p.cube, d, r * p.levels);"
				"}";
		}

	private:
		CubeMap cube;
		bool convoluted;

		void computeConv();
};

}
}

#endif // N_GRAPHICS_IBLPROBE_H
