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

//#define N_MATERIAL_FANCYNESS
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

	MaterialData() : color(1, 1, 1, 1), metallic(0), depthTested(true), depthWrite(true), blend(None), cull(Back), depth(Lesser), normalIntencity(0.5)/*, textures(0)*/
	#ifdef N_MATERIAL_FANCYNESS
		, fancy(true)
	#endif
	{
	}

	~MaterialData() {
		//delete textures;
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

	//core::Map<core::String, Texture> *textures;

	private:
		friend class Material;
		friend class internal::Material;

		#ifdef N_MATERIAL_FANCYNESS
		mutable bool fancy;

		void computeFancy() const {
			fancy =  blend != None ||
					 cull != Back ||
					 depth != Lesser ||
					 depthTested != true ||
					 depthWrite != true;
		}
		#endif

};

namespace internal {
	class Material : core::NonCopyable
	{

		const MaterialData &computeFancyness(const MaterialData &d) {
			#ifdef N_MATERIAL_FANCYNESS
			d.computeFancy();
			#endif
			return d;
		}

		public:
			Material(const MaterialData &d = MaterialData()) : data(computeFancyness(d)), index(0) {
			}

			~Material() {
				if(index) {
					mutex.lock();
					cache.remove(this);
					mutex.unlock();
				}
			}

			#define N_MAT_COMPARE(mem, op) if(data.mem != m.data.mem) { return data.mem op m.data.mem; }

			bool operator<(const Material &m) const {
				N_MAT_COMPARE(depthTested, >)
				N_MAT_COMPARE(depth, <)
				N_MAT_COMPARE(cull, <)
				N_MAT_COMPARE(prog, <)
				N_MAT_COMPARE(diffuse, <)
				N_MAT_COMPARE(depthWrite, <)
				return false;
			}

			#undef N_MAT_COMPARE

			const MaterialData data;

			uint getIndex() const {
				updateIfNeeded();
				return index;
			}

		private:
			static concurrent::Mutex mutex;
			static core::Array<const Material *> cache;

			void updateIfNeeded() const {
				if(!index) {
					mutex.lock();
					cache.append(this);
					cache.sort([](const Material *a, const Material *b) { return a->operator<(*b); });
					uint max = cache.size();
					for(uint i = 0; i != max; i++) {
						cache[i]->index = i + 1;
					}
					mutex.unlock();
				}
			}

			mutable uint index;
	};
}



class Material : private assets::Asset<internal::Material>
{
	public:
		Material() : assets::Asset<internal::Material>() {
		}

		Material(const MaterialData &i) : Material(new internal::Material(i)) {
		}

		bool operator<(const Material &m) const {
			return getInternalIndex() < m.getInternalIndex();
		}

		bool isValid() const {
			return assets::Asset<internal::Material>::isValid();
		}

		bool isNull() const {
			return assets::Asset<internal::Material>::isNull();
		}

		Color<> getColor() const {
			return getData().color;
		}

		MaterialData getData() const {
			#ifdef N_MATERIAL_CACHING
			if(cache.data.hasValue()) {
				return cache.data;
			}
			#endif
			const internal::Material *i = getInternal();
			if(i) {
				return
				#ifdef N_MATERIAL_CACHING
					cache.data =
				#endif
					i->data;
			}
			return MaterialData();
		}

	private:
		friend class MaterialLoader;
		friend class GLContext;

		friend class FrameBuffer;

		template<typename T>
		friend class VertexArrayObject;

		void bind(uint flags = RenderFlag::None) const;

		Material(const assets::Asset<internal::Material> &t) : assets::Asset<internal::Material>(t) {
		}

		Material(internal::Material *i) : assets::Asset<internal::Material>(std::move(i)) {
		}

		N_FORCE_INLINE const internal::Material *getInternal() const {
			return this->isValid() ? this->operator->() : (const internal::Material *)0;
		}

		uint getInternalIndex() const {
			if(!cache.index) {
				const internal::Material *i = getInternal();
				return cache.index = (i ? i->getIndex() : 0);
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

