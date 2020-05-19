/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include "SimpleMaterialData.h"

#include <yave/device/Device.h>

namespace yave {

static DescriptorSet create_descriptor_set(DevicePtr dptr, const SimpleMaterialData& data) {

	std::array<Descriptor, SimpleMaterialData::texture_count + 1> bindings = {
			*dptr->device_resources()[DeviceResources::GreyTexture],
			*dptr->device_resources()[DeviceResources::FlatNormalTexture],
			*dptr->device_resources()[DeviceResources::RedTexture],
			*dptr->device_resources()[DeviceResources::RedTexture],
			InlineDescriptor(data.constants())
		};

	for(usize i = 0; i != SimpleMaterialData::texture_count; ++i) {
		y_debug_assert(!data.textures()[i].is_loading());
		if(const auto* tex = data.textures()[i].get()) {
			bindings[i] = *tex;
		}
	}
	return DescriptorSet(dptr, bindings);
}


Material::Material(DevicePtr dptr, SimpleMaterialData&& data) :
		_template(dptr->device_resources()[DeviceResources::TexturedMaterialTemplate]),
		_set(create_descriptor_set(device(), data)),
		_data(std::move(data)) {
}

Material::Material(const MaterialTemplate* tmp, SimpleMaterialData&& data) :
		_template(tmp),
		_set(create_descriptor_set(device(), data)),
		_data(std::move(data)) {
}

const SimpleMaterialData& Material::data() const {
	return _data;
}

const DescriptorSetBase& Material::descriptor_set() const {
	return _set;
}

const MaterialTemplate* Material::material_template() const {
	return _template;
}

DevicePtr Material::device() const {
	Y_TODO(cache ?)
	return _template->device();
}

bool Material::is_null() const {
	return !_template;
}

}
