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
#ifndef YAVE_SHADER_SHADERPROGRAM_H
#define YAVE_SHADER_SHADERPROGRAM_H

#include "ShaderModule.h"

#include <y/core/AssocVector.h>

namespace yave {

class ShaderProgram : NonCopyable {

	public:
		ShaderProgram(DevicePtr dptr, const core::Vector<SpirVData>& modules);

		core::Vector<vk::PipelineShaderStageCreateInfo> vk_pipeline_stage_info() const;
		const core::Vector<vk::DescriptorSetLayout>& descriptor_layouts() const;

	private:
		core::AssocVector<vk::ShaderStageFlagBits, ShaderModule> _modules;
		core::Vector<vk::DescriptorSetLayout> _layouts;
};

}

#endif // YAVE_SHADER_SHADERPROGRAM_H
