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
#include "GraphicPipeline.h"

#include <yave/Device.h>

namespace yave {

GraphicPipeline::GraphicPipeline() {
}

GraphicPipeline::GraphicPipeline(DevicePtr dptr, vk::Pipeline pipeline, vk::PipelineLayout layout, const DescriptorSet& ds) :
		DeviceLinked(dptr),
		_descriptor_set(ds),
		_pipeline(pipeline),
		_layout(layout)  {
}

GraphicPipeline::~GraphicPipeline() {
	_descriptor_set.destroy(get_device());
	destroy(_pipeline);
	destroy(_layout);
}

GraphicPipeline::GraphicPipeline(GraphicPipeline&& other) : GraphicPipeline() {
	swap(other);
}

GraphicPipeline& GraphicPipeline::operator=(GraphicPipeline&& other) {
	swap(other);
	return *this;
}

void GraphicPipeline::swap(GraphicPipeline& other) {
	DeviceLinked::swap(other);
	std::swap(_pipeline, other._pipeline);
	std::swap(_layout, other._layout);
	std::swap(_descriptor_set, other._descriptor_set);
}

vk::Pipeline GraphicPipeline::get_vk_pipeline() const {
	return _pipeline;
}

vk::PipelineLayout GraphicPipeline::get_vk_pipeline_layout() const {
	return _layout;
}

const DescriptorSet& GraphicPipeline::get_descriptor_set() const {
	return _descriptor_set;
}


}
