/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_MATERIAL_MATERIALDATA_H
#define YAVE_MATERIAL_MATERIALDATA_H

#include <yave/yave.h>

#include <yave/assets/AssetPtr.h>

#include <yave/graphics/bindings/Binding.h>
#include <yave/graphics/shaders/SpirVData.h>

namespace yave {

enum class PrimitiveType {
	Triangles = uenum(vk::PrimitiveTopology::eTriangleList),
	Lines = uenum(vk::PrimitiveTopology::eLineList)
};

class MaterialData {
	public:
		MaterialData& set_frag_data(const SpirVData& data);
		MaterialData& set_vert_data(const SpirVData& data);
		MaterialData& set_geom_data(const SpirVData& data);

		MaterialData& set_bindings(const core::ArrayView<Binding>& binds);
		MaterialData& add_binding(const Binding& bind);

		MaterialData& keep_alive(const GenericAssetPtr& asset);

		MaterialData& set_primitive_type(PrimitiveType type);

		MaterialData& set_depth_tested(bool tested);
		MaterialData& set_culled(bool culled);
		MaterialData& set_blended(bool blended);

		core::Vector<Binding> _bindings;
		core::Vector<GenericAssetPtr> _keep_alive;

		SpirVData _frag;
		SpirVData _vert;
		SpirVData _geom;

		PrimitiveType _primitive_type = PrimitiveType::Triangles;

		bool _depth_tested = true;
		bool _cull = true;
		bool _blend = false;
};

}

#endif // YAVE_MATERIAL_MATERIALDATA_H
