/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

#include "MaterialData.h"

namespace yave {

MaterialData& MaterialData::set_frag_data(const SpirVData& data) {
	y_debug_assert(ShaderModuleBase::shader_type(data) == ShaderType::Fragment);
	_frag = data;
	return *this;
}

MaterialData& MaterialData::set_vert_data(const SpirVData& data) {
	y_debug_assert(ShaderModuleBase::shader_type(data) == ShaderType::Vertex);
	_vert = data;
	return *this;
}

MaterialData& MaterialData::set_geom_data(const SpirVData& data) {
	y_debug_assert(ShaderModuleBase::shader_type(data) == ShaderType::Geomery);
	_geom = data;
	return *this;
}

MaterialData& MaterialData::set_bindings(const core::ArrayView<Binding>& binds) {
	_bindings.assign(binds.begin(), binds.end());
	return *this;
}

MaterialData& MaterialData::add_binding(const Binding& bind) {
	_bindings.push_back(bind);
	return *this;
}

MaterialData& MaterialData::keep_alive(const GenericAssetPtr& asset) {
	_keep_alive.emplace_back(asset);
	return *this;
}

MaterialData& MaterialData::set_primitive_type(PrimitiveType type) {
	_primitive_type = type;
	return *this;
}

MaterialData& MaterialData::set_depth_tested(bool tested) {
	_depth_tested = tested;
	return *this;
}

MaterialData& MaterialData::set_culled(bool culled) {
	_cull = culled;
	return *this;
}

MaterialData& MaterialData::set_blended(bool blended) {
	_blend = blended;
	return *this;
}

}
