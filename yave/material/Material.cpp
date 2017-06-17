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
#include "Material.h"
#include "MaterialCompiler.h"
#include "MaterialData.h"

#include <yave/device/Device.h>

namespace yave {

Material::Material(DevicePtr dptr, const MaterialData &data) :
		DeviceLinked(dptr),
		_data(data),
		_set(dptr, data._bindings) {
}

Material::~Material() {
	_compiled.clear();
}

Material::Material(Material&& other) {
	swap(other);
}

Material& Material::operator=(Material&& other) {
	swap(other);
	return *this;
}

void Material::swap(Material& other) {
	DeviceLinked::swap(other);
	std::swap(_data, other._data);
	std::swap(_set, other._set);
	std::swap(_compiled, other._compiled);
}

const GraphicPipeline& Material::compile(const RenderPass& render_pass) {
	auto key = render_pass.vk_render_pass();
	if(!key) {
		fatal("Unable to compile material: null renderpass.");
	}

	auto it = std::find_if(_compiled.begin(), _compiled.end(), [=](const auto& c) { return c.first == key; });
	if(it == _compiled.end()) {
		MaterialCompiler compiler(device());
		_compiled.insert(key, compiler.compile(*this, render_pass));
		return _compiled.last().second;
	}
	return it->second;
}

const MaterialData& Material::data() const {
	return _data;
}

const DescriptorSet& Material::descriptor_set() const {
	return _set;
}

}
