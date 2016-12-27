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
#ifndef YAVE_MATERIAL_GRAPHICPIPELINE_H
#define YAVE_MATERIAL_GRAPHICPIPELINE_H


#include <yave/yave.h>
#include <yave/DeviceLinked.h>


namespace yave {

class Material;

class GraphicPipeline : NonCopyable {
	public:
		GraphicPipeline() = default;
		GraphicPipeline(const Material& mat, vk::Pipeline pipeline, vk::PipelineLayout layout);

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
