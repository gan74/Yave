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

#ifndef N_GRAPHICS_MATERIALDATA_H
#define N_GRAPHICS_MATERIALDATA_H

#include <n/utils.h>
#include "Texture.h"
#include "UniformBuffer.h"
#include "ShaderProgram.h"

namespace n {
namespace graphics {

enum RenderFlag : uint32
{
	None = 0x00,
	FastDepth = 0x01,
	ForceMaterialRebind = 0x02,
	NoShader = 0x04,
	DepthWriteOnly = 0x08,
	Overlay = 0x10

};


struct MaterialData
{

	static constexpr uint DefaultPriority = 1 << 22;

	MaterialData() : color(1, 1, 1, 1), metallic(0), normalIntencity(0.5), fancy{DefaultPriority, true, true, true, None, Lesser, Back} {
	}

	Color<> color;
	float metallic;

	graphics::ShaderProgram prog;

	Texture diffuse;
	Texture normal;
	Texture roughness;
	float normalIntencity;

	DynamicBufferBase uniforms;

	struct Fancy
	{
		uint32 renderPriority : 23; // uint23
		uint32 depthTested    : 1; // bool
		uint32 depthWrite     : 1; // bool
		uint32 depthSortable  : 1; // bool
		uint32 blend          : 2; // BlendMode
		uint32 depth          : 2; // DepthMode
		uint32 cull           : 2; // CullMode
	} fancy;

	uint32 fancy32() const {
		const void *wf = &fancy;
		return *reinterpret_cast<const uint32 *>(wf);
	}

	static_assert(sizeof(MaterialData::fancy) == 4, "MaterialData::fancy should be 32 bit.");
};

}
}

#endif // N_GRAPHICS_MATERIALDATA_H
