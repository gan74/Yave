/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#include "Material.h"
#include "MaterialCompiler.h"
#include "MaterialData.h"

#include <yave/Device.h>

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

const GraphicPipeline& Material::compile(const RenderPass& render_pass, const Viewport& viewport) {
	auto key = render_pass.get_vk_render_pass();
	auto it = core::range(_compiled).find(key);
	if(it == _compiled.end()) {
		MaterialCompiler compiler(get_device());
		_compiled.insert(key, compiler.compile(*this, render_pass, viewport));
		return _compiled.last().second;
	}
	return it->second;
}

const MaterialData& Material::get_data() const {
	return _data;
}

const DescriptorSet& Material::descriptor_set() const {
	return _set;
}

}
