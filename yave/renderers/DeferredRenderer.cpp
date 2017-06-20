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
#include "RenderingPipeline.h"

#include <yave/commands/CmdBufferRecorder.h>
#include <y/io/File.h>

#include <random>

namespace yave {

static ComputeShader create_lighting_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("deferred.comp.spv").expected("Unable to open SPIR-V file.")));
}

static auto create_lights(DevicePtr dptr, usize pts_count) {
	usize light_count = 1 + pts_count;
	using Vec4u32 = math::Vec<4, u32>;
	Buffer<BufferUsage::StorageBit, MemoryFlags::CpuVisible> buffer(dptr, sizeof(Vec4u32) + light_count * sizeof(uniform::Light));

	std::mt19937 gen(3);
	std::uniform_real_distribution<float> distr(0, 1);
	std::uniform_real_distribution<float> pos_distr(-1, 1);

	core::Vector<uniform::Light> lights;

	// sun
	lights << uniform::Light {
			math::Vec3{0, 0.5, 1}.normalized(), 0,
			math::Vec3{1, 1, 1} * 0.8,
			uniform::LightType::Directional
		};

	// fill
	lights << uniform::Light {
			-math::Vec3{0, 0.5, 1}.normalized(), 0,
			math::Vec3{1, 1, 1} * 0.25,
			uniform::LightType::Directional
		};

	for(usize i = 0; i != pts_count; i++) {
		lights << uniform::Light {
				{pos_distr(gen), pos_distr(gen), pos_distr(gen)},
				2,
				{distr(gen), distr(gen), distr(gen)},
				uniform::LightType::Point
			};
	}


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





DeferredRenderer::DeferredRenderer(const Ptr<GBufferRenderer>& gbuffer) :
		BufferRenderer(gbuffer->device()),
		_gbuffer(gbuffer),
		_lighting_shader(create_lighting_shader(device())),
		_lighting_program(_lighting_shader),
		_acc_buffer(device(), ImageFormat(vk::Format::eR16G16B16A16Sfloat), _gbuffer->size()),
		_lights_buffer(create_lights(device(), 1)),
		_camera_buffer(device(), 1),
		_descriptor_set(device(), {Binding(_gbuffer->depth()), Binding(_gbuffer->color()), Binding(_gbuffer->normal()), Binding(_camera_buffer), Binding(_lights_buffer), Binding(StorageView(_acc_buffer))}) {

	for(usize i = 0; i != 3; i++) {
		if(size()[i] % _lighting_shader.local_size()[i]) {
			log_msg("Compute local size at index "_s + i + " does not divide output buffer size.", Log::Warning);
		}
	}
}

const math::Vec2ui& DeferredRenderer::size() const {
	return _acc_buffer.size();
}

void DeferredRenderer::build_frame_graph(RenderingNode<result_type>& node, CmdBufferRecorder<>& recorder) {
	node.add_dependency(_gbuffer, recorder);
	auto culling = node.add_dependency(_gbuffer->scene_renderer()->culling_node());

	node.set_func([=, &recorder]() -> result_type {
			_camera_buffer.map()[0] = _gbuffer->scene_view().camera();
			recorder.dispatch(_lighting_program, math::Vec3ui(size() / _lighting_shader.local_size().to<2>(), 1), {_descriptor_set});
			return _acc_buffer;
		});
}

}
