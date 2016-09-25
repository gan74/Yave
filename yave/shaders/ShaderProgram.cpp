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

#include "ShaderProgram.h"
#include <unordered_map>

#include <iostream>

namespace yave {

using SSR = ShaderStageResource;

void add_resource(core::Vector<SSR>& resources, const ShaderResource& r, vk::ShaderStageFlags stage) {
	auto it = core::range(resources).find([&](const SSR& res) {
		return res.resource == r;
	});
	if(it == resources.end()) {
		resources << SSR{r, stage};
	} else {
		it->stage |= stage;
	}
}

void add_resources(core::Vector<SSR>& resources, const ShaderModule& mod) {
	for(const auto& r : mod.shader_resources()) {
		add_resource(resources, r, mod.shader_stage());
	}
}

void print(const core::Vector<DescriptorLayout>& layouts) {
	for(const auto& lay : layouts) {
		std::cout << "--------------------------------" << std::endl;
		std::cout << "set = " << lay.set_index() << std::endl << std::endl;
	}
}

ShaderProgram::ShaderProgram(core::Vector<ShaderModule>&& modules) : _modules(std::move(modules)) {
	DevicePtr dptr = nullptr;
	for(const auto& mod : _modules) {
		dptr = mod.get_device();
		add_resources(_resources, mod);
	}

	std::unordered_map<u32, core::Vector<ShaderStageResource>> sets;
	for(const auto& r : _resources) {
		sets[r.resource.set] << r;
	}
	for(const auto& set : sets) {
		_layouts << DescriptorLayout(dptr, set.second);
	}


	print(_layouts);
}

core::Vector<vk::PipelineShaderStageCreateInfo> ShaderProgram::get_vk_pipeline_stage_info() const {
	core::Vector<vk::PipelineShaderStageCreateInfo> stage_create_infos;
	for(const auto& mod : _modules) {
		stage_create_infos << vk::PipelineShaderStageCreateInfo()
				.setModule(mod.get_vk_shader_module())
				.setStage(mod.shader_stage())
				.setPName("main")
			;
	}
	return stage_create_infos;
}

}
