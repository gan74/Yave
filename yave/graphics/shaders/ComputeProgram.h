/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_SHADERS_COMPUTEPROGRAM_H
#define YAVE_GRAPHICS_SHADERS_COMPUTEPROGRAM_H

#include "ShaderModule.h"

namespace yave {

class ComputeProgram final {
    public:
        ComputeProgram() = default;
        ComputeProgram(ComputeProgram&&) = default;
        ComputeProgram& operator=(ComputeProgram&&) = default;

        explicit ComputeProgram(const ComputeShader& comp, const SpecializationData& data = SpecializationData());
        ~ComputeProgram();

        bool is_null() const;

        const math::Vec3ui& local_size() const;
        usize thread_count() const;

        VkPipeline vk_pipeline() const;
        VkPipelineLayout vk_pipeline_layout() const;

    private:
        VkHandle<VkPipelineLayout> _layout;
        VkHandle<VkPipeline> _pipeline;
        math::Vec3ui _local_size;
};

}

#endif // YAVE_GRAPHICS_SHADERS_COMPUTEPROGRAM_H

