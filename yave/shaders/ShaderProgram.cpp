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

ShaderProgram::ShaderProgram(const ShaderModule& frag, const ShaderModule& vert, const ShaderModule& geom) {
	add_resources(_resources, frag);
	add_resources(_resources, vert);
	add_resources(_resources, geom);

	std::unordered_map<u32, core::Vector<ShaderStageResource>> sets;
	for(const auto& r : _resources) {
		sets[r.resource.set] << r;
	}
	for(const auto& set : sets) {
		_layouts << DescriptorLayout(frag.get_device(), set.second);
	}


	print(_layouts);
}

}
