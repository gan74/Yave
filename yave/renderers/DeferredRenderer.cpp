/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "DeferredRenderer.h"
#include <yave/commands/CmdBufferRecorder.h>

#include <y/io/File.h>

#include <random>

namespace yave {

static constexpr vk::Format depth_format = vk::Format::eD32Sfloat;
static constexpr vk::Format diffuse_format = vk::Format::eR8G8B8A8Unorm;
static constexpr vk::Format normal_format = vk::Format::eR8G8B8A8Unorm;

static ComputeShader create_lighting_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("deferred.comp.spv").expected("Unable to open SPIR-V file.")));
}

static auto create_lights(DevicePtr dptr, usize dir_count, usize pts_count) {
	usize light_count = dir_count + pts_count + 1;
	using Vec4u32 = math::Vec<4, u32>;
	Buffer<BufferUsage::StorageBit, MemoryFlags::CpuVisible> buffer(dptr, sizeof(Vec4u32) + light_count * sizeof(uniform::Light));

	std::mt19937 gen(3);
	std::uniform_real_distribution<float> distr(0, 1);
	std::uniform_real_distribution<float> pos_distr(-1, 1);

	core::Vector<uniform::Light> lights;
	for(usize i = 0; i != dir_count; i++) {
		lights << uniform::Light {
				math::Vec3(0, 0, 1).normalized(), 0,
				math::Vec3(1, 1, 1) * 0.5,
				uniform::LightType::Directional
			};
	}
	for(usize i = 0; i != pts_count; i++) {
		lights << uniform::Light {
				math::Vec3(pos_distr(gen), pos_distr(gen), pos_distr(gen)),
				2,
				math::Vec3(distr(gen), distr(gen), distr(gen)),
				uniform::LightType::Point
			};
	}
	lights << uniform::Light {
			  math::Vec3(0, 0, 100),
			  0.1,
			  math::Vec3(1, 0, 1),
			  uniform::LightType::Point
		  };

	{
		TypedSubBuffer<Vec4u32, BufferUsage::StorageBit, MemoryFlags::CpuVisible>(buffer, 0, 1).map()[0] = Vec4u32(light_count);
	}
	{
		TypedSubBuffer<uniform::Light, BufferUsage::StorageBit, MemoryFlags::CpuVisible> light_buffer(buffer, sizeof(Vec4u32), lights.size());
		auto map = light_buffer.map();
		std::copy(lights.begin(), lights.end(), map.begin());
	}

	return buffer;
}





DeferredRenderer::DeferredRenderer(DevicePtr dptr, SceneView &view, const math::Vec2ui& size) :
		DeviceLinked(dptr),
		_scene_renderer(dptr, view),

		_size(size),
		_depth(device(), depth_format, _size),
		_diffuse(device(), diffuse_format, _size),
		_normal(device(), normal_format, _size),
		_gbuffer(device(), _depth, {_diffuse, _normal}),

		_lighting_shader(create_lighting_shader(device())),
		_lighting_program(_lighting_shader),

		_lights(create_lights(device(), 1, 1)),

		_camera_buffer(device(), 1),
		_lighting_set(device(), {Binding(_depth), Binding(_diffuse), Binding(_normal), Binding(_camera_buffer), Binding(_lights)}) {


	for(usize i = 0; i != 3; i++) {
		if(_size[i] % _lighting_shader.local_size()[i]) {
			log_msg("Compute local size at index "_s + i + " does not divide output buffer size.", LogType::Warning);
		}
	}
}




const DescriptorSet& DeferredRenderer::create_output_set(const StorageView& out) {
	if(out.size() != _size) {
		fatal("Invalid output image size.");
	}

	auto it = _output_sets.find(out.vk_image_view());
	if(it == _output_sets.end()) {
		it = _output_sets.insert(std::make_pair(out.vk_image_view(), DescriptorSet(device(), {
				Binding(out)
			}))).first;
	}
	return it->second;
}

void DeferredRenderer::process(FrameToken& token) {
	_camera_buffer.map()[0] = _scene_renderer.scene_view().camera();

	token.cmd_buffer.bind_framebuffer(_gbuffer);
	_scene_renderer.process(token);

	token.cmd_buffer.dispatch(_lighting_program, math::Vec3ui(_size / _lighting_shader.local_size().sub(3), 1), {_lighting_set, create_output_set(token.image_view)});
}


core::ArrayProxy<Node*> DeferredRenderer::dependencies() {
	return _scene_renderer.dependencies();
}

}
