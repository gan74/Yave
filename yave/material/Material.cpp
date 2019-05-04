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
#include "Material.h"
#include "BasicMaterialData.h"

#include <yave/device/Device.h>

namespace yave {

static DescriptorSet create_descriptor_set(DevicePtr dptr, const BasicMaterialData& data) {
	if(data.is_empty()) {
		return DescriptorSet();
	}
	auto bindings = core::vector_with_capacity<Binding>(data.textures().size());
	for(const AssetPtr<Texture>& tex : data.textures()) {
		bindings.emplace_back(tex ? *tex : *dptr->device_resources()[DeviceResources::BlackTexture]);
	}
	return DescriptorSet(dptr, bindings);
}


Material::Material(DevicePtr dptr, BasicMaterialData&& data) :
		_template(dptr->device_resources()[data.is_empty() ? DeviceResources::BasicMaterialTemplate : DeviceResources::TexturedMaterialTemplate]),
		_set(create_descriptor_set(device(), data)),
		_data(std::move(data)) {
}

Material::Material(const MaterialTemplate* tmp, BasicMaterialData&& data) :
		_template(tmp),
		_set(create_descriptor_set(device(), data)),
		_data(std::move(data)) {
}

const BasicMaterialData& Material::data() const {
	return _data;
}

const DescriptorSetBase& Material::descriptor_set() const {
	return _set;
}

const MaterialTemplate* Material::mat_template() const {
	return _template;
}

DevicePtr Material::device() const {
	return _template->device();
}

}
