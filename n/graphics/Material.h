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

#ifndef N_GRAPHICS_MATERIAL
#define N_GRAPHICS_MATERIAL

#include <n/math/Vec.h>
#include "Texture.h"
#include "ShaderProgram.h"

#define N_MATERIAL_CACHING

namespace n {
namespace graphics {

enum RenderFlag
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
	enum CullMode {
		DontCull = 2,
		Back = 0,
		Front = 1
	};

	enum BlendMode {
		None,
		Add,
		SrcAlpha
	};

	enum DepthMode {
		Lesser,
		Greater,
		Always
	};

	MaterialData() : color(1, 1, 1, 1), metallic(0), depthTested(true), depthWrite(true), blend(None), cull(Back), depth(Lesser), normalIntencity(0.5), index(0) {
	}

	~MaterialData() {
		removeFromCache();
	}

	Color<> color;
	float metallic;
	bool depthTested;
	bool depthWrite;

	graphics::ShaderProgram prog;

	BlendMode blend;
	CullMode cull;
	DepthMode depth;

	Texture diffuse;
	Texture normal;
	Texture roughness;
	float normalIntencity;

	private:
		friend class Material;

		mutable uint index;

		void addToCache();
		void removeFromCache();


};

class Material : public assets::Asset<MaterialData>
{
	public:
		Material() : assets::Asset<MaterialData>() {
		}

		Material(const MaterialData &i) : Material(new MaterialData(i)) {
		}


		Material(const MaterialData *i) : assets::Asset<MaterialData>(std::move(i)) {
		}

		bool operator<(const Material &m) const {
			return getInternalIndex() < m.getInternalIndex();
		}

		const MaterialData &getData() const;

	private:
		friend class MaterialLoader;
		friend class GLContext;

		friend class FrameBuffer;

		template<typename T>
		friend class VertexArrayObject;

		void bind(uint flags = RenderFlag::None) const;

		Material(const assets::Asset<MaterialData> &t) : assets::Asset<MaterialData>(t) {
		}

		N_FORCE_INLINE const MaterialData *getInternal() const {
			return this->isValid() ? this->operator->() : 0;
		}

		uint getInternalIndex() const {
			if(!cache.index) {
				const MaterialData *i = getInternal();
				return cache.index = (i ? i->index : 0);
			}
			return cache.index;
		}

		struct Cache
		{
			Cache() : index(0) {
			}

			uint index;
			#ifdef N_MATERIAL_CACHING
			core::Option<MaterialData> data;
			#endif
		};

		mutable Cache cache;

};

static_assert(!IsThreadSafe<Material>::value, "n::graphics::Material should not be treated as thread-safe");

}
}

#endif // N_GRAPHICS_MATERIAL

