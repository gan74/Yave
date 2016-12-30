/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#include <yave/descriptors/Binding.h>

#include <yave/shaders/SpirVData.h>

namespace yave {

struct MaterialData {

	SpirVData _frag;
	SpirVData _vert;
	SpirVData _geom;

	core::Vector<Binding> _bindings;

	MaterialData& set_frag_data(SpirVData&& data);
	MaterialData& set_vert_data(SpirVData&& data);
	MaterialData& set_geom_data(SpirVData&& data);

	MaterialData& set_bindings(const core::Vector<Binding>& binds);
};

}

#endif // YAVE_MATERIAL_MATERIAL_MATERIALDATA_H
