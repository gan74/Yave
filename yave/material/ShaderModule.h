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
#ifndef YAVE_MATERIAL_SHADERMODULE_H
#define YAVE_MATERIAL_SHADERMODULE_H

#include <yave/yave.h>
#include <yave/DeviceLinked.h>

#include "SpirVData.h"

namespace yave {

class ShaderModule : NonCopyable, public DeviceLinked {

	public:
		ShaderModule() = default;

		ShaderModule(DevicePtr dptr, vk::ShaderModule mod) : DeviceLinked(dptr), _module(mod) {
		}

		ShaderModule(ShaderModule&& other) : ShaderModule() {
			swap(other);
		}

		ShaderModule& operator=(ShaderModule&& other) {
			swap(other);
			return *this;
		}

		vk::ShaderModule get_vk_shader_module() const {
			return _module;
		}

		~ShaderModule() {
			destroy(_module);
		}

	private:
		void swap(ShaderModule& other) {
			DeviceLinked::swap(other);
			std::swap(_module, other._module);
		}

		vk::ShaderModule _module;

};


}

#endif // YAVE_MATERIAL_SHADERMODULE_H
