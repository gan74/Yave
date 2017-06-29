/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#include <yave/bindings/Binding.h>

#include <yave/shaders/SpirVData.h>

namespace yave {

enum class PrimitiveType {
	Triangles = uenum(vk::PrimitiveTopology::eTriangleList),
	Lines = uenum(vk::PrimitiveTopology::eLineList)
};

struct MaterialData {
	SpirVData _frag;
	SpirVData _vert;
	SpirVData _geom;

	PrimitiveType _primitive_type = PrimitiveType::Triangles;

	core::Vector<Binding> _bindings;

	MaterialData& set_frag_data(SpirVData&& data);
	MaterialData& set_vert_data(SpirVData&& data);
	MaterialData& set_geom_data(SpirVData&& data);

	MaterialData& set_bindings(const core::ArrayProxy<Binding>& binds);

	MaterialData& set_primitive_type(PrimitiveType type);
};

}

#endif // YAVE_MATERIAL_MATERIAL_MATERIALDATA_H
