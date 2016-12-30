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
#ifndef YAVE_SHADER_SHADERMODULEBASE_H
#define YAVE_SHADER_SHADERMODULEBASE_H

#include <yave/yave.h>
#include <yave/DeviceLinked.h>

#include "SpirVData.h"

#include <unordered_map>

namespace yave {

enum class ShaderType {
	None = 0,
	Fragment = uenum(vk::ShaderStageFlagBits::eFragment),
	Vertex = uenum(vk::ShaderStageFlagBits::eVertex),
	Geomery = uenum(vk::ShaderStageFlagBits::eGeometry),
	Compute = uenum(vk::ShaderStageFlagBits::eCompute)
};

class ShaderModuleBase : NonCopyable, public DeviceLinked {

	public:
		struct Attribute {
			u32 location;
			u32 columns;
			u32 vec_size;
			u32 component_size;
		};

		ShaderModuleBase() = default;

		ShaderModuleBase(DevicePtr dptr, const SpirVData& data);
		~ShaderModuleBase();

		const auto& bindings() const {
			return _bindings;
		}

		const auto& attributes() const {
			return _attribs;
		}

		ShaderType type() const {
			return _type;
		}

		vk::ShaderModule vk_shader_module() const;

	protected:
		void swap(ShaderModuleBase& other);

	private:
		vk::ShaderModule _module;
		ShaderType _type = ShaderType::None;
		std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>> _bindings;
		core::Vector<Attribute> _attribs;

};

}

#endif // YAVE_SHADER_SHADERMODULEBASE_H
