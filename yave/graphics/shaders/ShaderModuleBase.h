/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "SpirVData.h"

#include <yave/graphics/graphics.h>

#include <yave/utils/traits.h>

#include <y/core/HashMap.h>

namespace yave {

enum class ShaderType : u32 {
    None = 0,
    Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    Vertex = VK_SHADER_STAGE_VERTEX_BIT,
    Geomery = VK_SHADER_STAGE_GEOMETRY_BIT,
    Compute = VK_SHADER_STAGE_COMPUTE_BIT
};


class ShaderModuleBase : NonMovable {
    public:
        struct Attribute {
            u32 location;
            u32 component_count;
            VkFormat format;
            bool is_packed;
        };

        ~ShaderModuleBase();

        core::Span<Attribute> attributes() const {
            return _attribs;
        }

        ShaderType type() const {
            return _type;
        }

        const math::Vec3ui& local_size() const {
            return _local_size;
        }

        VkShaderModule vk_shader_module() const {
            return _module;
        }

        core::Span<u32> stage_output() const {
            return _stage_output;
        }

        const core::String& entry_point() const {
            return _entry_point;
        }

    protected:
        ShaderModuleBase() = default;

        ShaderModuleBase(const SpirVData& data, ShaderType type);

    private:
        friend class ShaderProgram;
        friend class ComputeProgram;

        const auto& bindings() const {
            return _bindings;
        }

        core::Span<u32> variable_size_bindings() const {
            return _variable_size_bindings;
        }

    private:
        VkHandle<VkShaderModule> _module;
        core::String _entry_point = "main";
        ShaderType _type = ShaderType::None;
        core::FlatHashMap<u32, core::Vector<VkDescriptorSetLayoutBinding>> _bindings;
        core::Vector<u32> _variable_size_bindings;
        core::Vector<Attribute> _attribs;
        core::Vector<u32> _stage_output;
        math::Vec3ui _local_size;


};

static_assert(is_safe_base<ShaderModuleBase>::value);

}

#endif // YAVE_GRAPHICS_SHADERS_SHADERMODULEBASE_H

