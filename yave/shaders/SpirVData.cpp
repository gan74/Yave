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
#include "SpirVData.h"

#include <spirv-cross/spirv.hpp>
#include <spirv-cross/spirv_cross.hpp>
#include <spirv-cross/spirv_glsl.hpp>

namespace yave {

void build_resources(
		core::Vector<ShaderResource>& out,
		ShaderResourceType type,
		const std::vector<spirv_cross::Resource>& resources,
		spirv_cross::Compiler& compiler) {

	for(auto r : resources) {
		auto id = r.id;
		out << ShaderResource {
				type,
				core::str(r.name),
				compiler.get_decoration(id, spv::DecorationDescriptorSet),
				compiler.get_decoration(id, spv::DecorationBinding)
			};
	}
}

SpirVData SpirVData::from_file(io::ReaderRef reader) {
	Y_TODO(optimise)
	core::Vector<u8> content = reader->read_all();
	core::Vector<u32> spriv32(content.size() / 4, 0);
	memcpy(spriv32.begin(), content.begin(), content.size());
	return SpirVData(spriv32);
}



SpirVData::SpirVData(const core::Vector<u32>& data) : _data(data), _stage(vk::ShaderStageFlagBits::eAll) {
	if(!is_empty()) {
		spirv_cross::CompilerGLSL glsl(std::vector<u32>(_data.begin(), _data.end()));
		spirv_cross::ShaderResources glsl_resources = glsl.get_shader_resources();

		build_resources(_resources, ShaderResourceType::UniformBuffer, glsl_resources.uniform_buffers, glsl);
		build_resources(_resources, ShaderResourceType::Texture, glsl_resources.sampled_images, glsl);


		switch(glsl.get_execution_model()) {
			case spv::ExecutionModelVertex:
				_stage = vk::ShaderStageFlagBits::eVertex;
				break;
			case spv::ExecutionModelFragment:
				_stage = vk::ShaderStageFlagBits::eFragment;
				break;
			case spv::ExecutionModelGeometry:
				_stage = vk::ShaderStageFlagBits::eGeometry;
				break;
			default:
				_stage = vk::ShaderStageFlagBits::eAll;
				log_msg("Unknown shader stage, setting to \"all\"", LogType::Warning);
		}
	}
}

const core::Vector<ShaderResource>& SpirVData::shader_resources() const {
	return _resources;
}

vk::ShaderStageFlags SpirVData::shader_stage() const {
	return _stage;
}

usize SpirVData::size() const {
	return _data.size() * 4;
}

const u32* SpirVData::data() const {
	return _data.begin();
}

bool SpirVData::is_empty() const {
	return _data.is_empty();
}

}
