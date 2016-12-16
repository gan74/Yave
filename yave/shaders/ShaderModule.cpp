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

#include "ShaderModule.h"

#include <yave/Device.h>

namespace yave {

vk::ShaderModule create_shader_module(DevicePtr dptr, const SpirVData& data) {
	if(data.is_empty()) {
		return VK_NULL_HANDLE;
	}
	return dptr->vk_device().createShaderModule(vk::ShaderModuleCreateInfo()
			.setCodeSize(data.size())
			.setPCode(data.data())
		);
}


ShaderModule::ShaderModule(DevicePtr dptr, const SpirVData& data) : DeviceLinked(dptr), _module(create_shader_module(dptr, data)) {
}

ShaderModule::ShaderModule(ShaderModule&& other) : ShaderModule() {
	swap(other);
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) {
	swap(other);
	return *this;
}

vk::ShaderModule ShaderModule::vk_shader_module() const {
	return _module;
}

ShaderModule::~ShaderModule() {
	destroy(_module);
}

void ShaderModule::swap(ShaderModule& other) {
	DeviceLinked::swap(other);
	std::swap(_module, other._module);
}


}
