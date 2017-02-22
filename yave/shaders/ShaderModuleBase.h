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
#ifndef YAVE_SHADER_SHADERMODULEBASE_H
#define YAVE_SHADER_SHADERMODULEBASE_H

#include <yave/yave.h>
#include <yave/device/DeviceLinked.h>

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

		const math::Vec3ui& local_size() const {
			return _local_size;
		}

		vk::ShaderModule vk_shader_module() const {
			return _module;
		}

	protected:
		void swap(ShaderModuleBase& other);

	private:
		vk::ShaderModule _module;
		ShaderType _type = ShaderType::None;
		std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>> _bindings;
		core::Vector<Attribute> _attribs;
		math::Vec3ui _local_size;

};

}

#endif // YAVE_SHADER_SHADERMODULEBASE_H
