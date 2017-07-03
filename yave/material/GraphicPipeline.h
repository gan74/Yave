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
#ifndef YAVE_MATERIAL_GRAPHICPIPELINE_H
#define YAVE_MATERIAL_GRAPHICPIPELINE_H

#include <yave/vk/vk.h>

#include <yave/device/DeviceLinked.h>

namespace yave {

class Material;

class GraphicPipeline : NonCopyable {

	public:
		GraphicPipeline() = default;
		GraphicPipeline(const Material* mat, vk::Pipeline pipeline, vk::PipelineLayout layout);

		~GraphicPipeline();

		GraphicPipeline(GraphicPipeline&& other);
		GraphicPipeline& operator=(GraphicPipeline&& other);

		vk::Pipeline vk_pipeline() const;
		vk::PipelineLayout vk_pipeline_layout() const;
		const vk::DescriptorSet& vk_descriptor_set() const;

	private:
		void swap(GraphicPipeline& other);

		const Material* _material = nullptr;

		vk::Pipeline _pipeline;
		vk::PipelineLayout _layout;
};

}

#endif // YAVE_MATERIAL_GRAPHICPIPELINE_H
