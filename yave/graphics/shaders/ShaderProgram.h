/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_SHADERS_SHADERPROGRAM_H
#define YAVE_GRAPHICS_SHADERS_SHADERPROGRAM_H

#include "ShaderModule.h"

#include <yave/graphics/buffers/Buffer.h>

#include <y/core/HashMap.h>

namespace yave {

class ShaderProgramBase : NonCopyable {
    public:
        ShaderProgramBase() = default;

        core::Span<VkPipelineShaderStageCreateInfo> vk_pipeline_stage_info() const;
        core::Span<VkDescriptorSetLayout> vk_descriptor_layouts() const;

    protected:
        ShaderProgramBase(core::Span<const ShaderModuleBase*> shaders);

        ShaderProgramBase(ShaderProgramBase&& other);
        ShaderProgramBase& operator=(ShaderProgramBase&& other);

        void swap(ShaderProgramBase& other);

        core::FlatHashMap<u32, core::Vector<VkDescriptorSetLayoutBinding>> _bindings;
        core::Vector<VkDescriptorSetLayout> _layouts;
        core::Vector<VkPipelineShaderStageCreateInfo> _stages;
};



class ShaderProgram final : public ShaderProgramBase {

    public:
        static constexpr u32 per_instance_location = 8;
        static constexpr u32 per_instance_binding = per_instance_location;

        ShaderProgram() = default;
        ShaderProgram(const FragmentShader& frag, const VertexShader& vert, const GeometryShader& geom);

        // should ALWAYS be sorted by location
        core::Span<VkVertexInputBindingDescription> vk_attribute_bindings() const;
        core::Span<VkVertexInputAttributeDescription> vk_attributes_descriptions() const;

        core::Span<u32> fragment_outputs() const;

    private:
        core::Vector<u32> _fragment_outputs;

        struct {
            core::Vector<VkVertexInputAttributeDescription> attribs;
            core::Vector<VkVertexInputBindingDescription> bindings;
        } _vertex;
};


class RaytracingProgram final : public ShaderProgramBase {
    public:
        RaytracingProgram() = default;
        RaytracingProgram(RaytracingProgram&& other) = default;
        RaytracingProgram& operator=(RaytracingProgram&& other) = default;

        RaytracingProgram(const RayGenShader& gen, const MissShader& miss, const ClosestHitShader& chit);
        ~RaytracingProgram();

        VkPipeline vk_pipeline() const;
        VkPipelineLayout vk_pipeline_layout() const;

        std::array<VkStridedDeviceAddressRegionKHR, 4> vk_binding_tables() const;

    private:
        VkHandle<VkPipelineLayout> _layout;
        VkHandle<VkPipeline> _pipeline;
        Buffer<BufferUsage::BindingTableBit, MemoryType::CpuVisible> _binding_tables;
};



}

#endif // YAVE_GRAPHICS_SHADERS_SHADERPROGRAM_H

