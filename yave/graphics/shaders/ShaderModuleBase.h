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
#ifndef YAVE_GRAPHICS_SHADERS_SHADERMODULEBASE_H
#define YAVE_GRAPHICS_SHADERS_SHADERMODULEBASE_H

#include <yave/graphics/vk/vk.h>

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

class SpecializationData : NonCopyable {
		template<typename... Args>
		static std::true_type is_tuple(const std::tuple<Args...>&);

		template<typename T>
		static std::false_type is_tuple(const T&);

	public:
		SpecializationData() = default;

		template<typename T>
		SpecializationData(const T& data) : _data(&data), _size(sizeof(T)) {
			static_assert(!decltype(is_tuple(data))::value, "std::tuple is not standard layout");
		}

		template<typename T>
		SpecializationData(const core::ArrayView<T>& arr) : _data(arr.data()), _size(arr.size(), sizeof(T)) {
			static_assert(!decltype(is_tuple(*arr.data()))::value, "std::tuple is not standard layout");
		}

		const void* data() const {
			return _data;
		}

		usize size() const {
			return _size;
		}

	private:
		const void* _data = nullptr;
		usize _size = 0;
};

class ShaderModuleBase : NonCopyable, public DeviceLinked {

	public:
		enum class AttribType {
			Uint = 0,
			Int = 1,
			Float = 2,
			Char = 3
		};

		struct Attribute {
			u32 location;
			u32 columns;
			u32 vec_size;
			u32 component_size;
			AttribType type;
		};

		~ShaderModuleBase();

		const auto& bindings() const {
			return _bindings;
		}

		const auto& attributes() const {
			return _attribs;
		}

		const auto& push_constants() const {
			return _push_constants;
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

		usize specialization_data_size() const {
			return _spec_constants.is_empty() ? 0 : (_spec_constants.last().offset + _spec_constants.last().size);
		}

		const auto& specialization_entries() const {
			return _spec_constants;
		}

		static ShaderType shader_type(const SpirVData& data);

	protected:
		ShaderModuleBase() = default;

		ShaderModuleBase(DevicePtr dptr, const SpirVData& data);

		void swap(ShaderModuleBase& other);

	private:
		vk::ShaderModule _module;
		ShaderType _type = ShaderType::None;
		std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>> _bindings;
		core::Vector<vk::SpecializationMapEntry> _spec_constants;
		core::Vector<vk::PushConstantRange> _push_constants;
		core::Vector<Attribute> _attribs;
		math::Vec3ui _local_size;

};

static_assert(is_safe_base<ShaderModuleBase>::value);

}

#endif // YAVE_GRAPHICS_SHADERS_SHADERMODULEBASE_H
