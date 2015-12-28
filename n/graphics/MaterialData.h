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
#include "DynamicBuffer.h"
#include "ShaderProgram.h"

namespace n {
namespace graphics {

enum RenderFlag : uint32
{
	None = 0x00,
	FastDepth = 0x01,
	// ???  = 0x02,
	NoShader = 0x04,
	DepthWriteOnly = 0x08,
	Overlay = 0x10,
	NoUniforms = 0x20
};

struct MaterialBufferData
{
	math::Vec4 color;

	float metallic;
	float diffuseIntencity;
	float normalIntencity;
	float roughnessIntencity;

	uint64 diffuse;
	uint64 normal;
	uint64 roughness;
	float padding[2];
};

struct MaterialSurfaceData
{
	MaterialSurfaceData();
	MaterialBufferData toBufferData() const;
	bool synchronize(bool immediate = false) const;

	Color<> color;
	float metallic;

	Texture diffuse;
	Texture normal;
	Texture roughness;
	float normalIntencity;
};

struct MaterialRenderData
{
	static constexpr uint32 DefaultPriority = 1 << 22;
	static constexpr uint32 PriorityMask = ~0x01FF;

	MaterialRenderData();
	uint32 toUint() const;
	void bind(uint flags = None) const;

	uint32 renderPriority : 23; // uint23
	uint32 depthTested    : 1; // bool
	uint32 depthWrite     : 1; // bool
	uint32 depthSortable  : 1; // bool
	uint32 blendMode      : 2; // BlendMode
	uint32 depthMode      : 2; // DepthMode
	uint32 cullMode       : 2; // CullMode
};

static_assert(sizeof(MaterialRenderData) == 4, "MaterialRenderData should be 32 bit.");
static_assert(sizeof(MaterialBufferData) % 16 == 0, "sizeof(MaterialBufferData) should be a multiple of 16 bytes.");

struct MaterialData
{
	void bind(uint flags = None) const;
	bool canInstanciate(const MaterialData &m) const;
	bool canInstanciate(const MaterialRenderData &r, const ShaderProgram &p) const;
	bool synchronize(bool immediate = false) const;

	bool operator<(const MaterialData &m) const {
		return render.toUint() < m.render.toUint();
	}

	graphics::ShaderProgram prog;
	MaterialSurfaceData surface;
	MaterialRenderData render;
};

}
}

#endif // N_GRAPHICS_MATERIALDATA_H
