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
#ifndef YAVE_SHADER_SHADERMODULE_H
#define YAVE_SHADER_SHADERMODULE_H

#include "ShaderModuleBase.h"

namespace yave {

template<ShaderType Type>
class ShaderModule : public ShaderModuleBase {

	static_assert(Type != ShaderType::None, "ShaderModule can not have None Type");

	public:
		ShaderModule() = default;

		ShaderModule(DevicePtr dptr, const SpirVData& data) : ShaderModuleBase(dptr, data) {
			if(type() != ShaderType::None && type() != Type) {
				fatal("Spirv data doesn't match ShaderModule Type.");
			}
		}

		ShaderModule(ShaderModule&& other) : ShaderModule() {
			swap(other);
		}

		ShaderModule& operator=(ShaderModule&& other) {
			swap(other);
		}

	private:

};

using FragmentShader = ShaderModule<ShaderType::Fragment>;
using VertexShader = ShaderModule<ShaderType::Vertex>;
using GeometryShader = ShaderModule<ShaderType::Geomery>;
using ComputeShader = ShaderModule<ShaderType::Compute>;

}

#endif // YAVE_SHADER_SHADERMODULE_H
