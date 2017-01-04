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


static ComputeShader create_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("deferred.comp.spv")));
}

static auto create_lights(DevicePtr dptr, usize count) {
	TypedBuffer<u8, BufferUsage::StorageBit, MemoryFlags::CpuVisible> buffer(dptr, sizeof(u32) + count * sizeof(DeferredRenderer::Light));
	auto map = buffer.map();

	std::mt19937 gen(3);
	std::uniform_real_distribution<float> distr(0, 1);
	std::uniform_real_distribution<float> pos_distr(-1, 1);

	core::Vector<DeferredRenderer::Light> lights;
	for(usize i = 0; i != count; i++) {
		lights << DeferredRenderer::Light {
				math::Vec3(distr(gen), distr(gen), distr(gen)), 1,
				math::Vec3(pos_distr(gen), pos_distr(gen), pos_distr(gen)).normalized(), 0
			};
		log_msg("lights["_s + i + "].color     = (" + lights.last().color.x() + ", " + lights.last().color.y() + ", " + lights.last().color.z() + ")");
		log_msg("lights["_s + i + "].direction = (" + lights.last().direction.x() + ", " + lights.last().direction.y() + ", " + lights.last().direction.z() + ")");
	}

	math::Vec<4, u32> count32(count);
	memcpy(map.begin(), &count32, sizeof(count32));
	memcpy(map.begin() + sizeof(count32), lights.begin(), sizeof(DeferredRenderer::Light) * count);

	return buffer;
}

DeferredRenderer::DeferredRenderer(SceneView &scene, const math::Vec2ui& size) :
		DeviceLinked(scene.device()),
		_scene(scene),
		_size(size),
		_depth(device(), depth_format, _size),
		_diffuse(device(), diffuse_format, _size),
		_normal(device(), normal_format, _size),
		_gbuffer(device(), _depth, {_diffuse, _normal}),
		_lights(create_lights(device(), 2)),
		_shader(create_shader(device())),
		_program(_shader) {

	for(usize i = 0; i != 3; i++) {
		if(_size[i] % _shader.local_size()[i]) {
			log_msg("Compute local size at index "_s + i + " does not divide output buffer size.", LogType::Warning);
		}
	}
}

void DeferredRenderer::draw(CmdBufferRecorder& recorder, const OutputView& out) {
	recorder.bind_framebuffer(_gbuffer);
	_scene.draw(recorder);

	recorder.image_barriers({_depth, _diffuse, _normal}, PipelineStage::AttachmentOutBit, PipelineStage::ComputeBit);
	recorder.dispatch(_program, math::Vec3ui(_size / _shader.local_size().sub(3), 1), create_descriptor_set(out));
}

const DescriptorSet& DeferredRenderer::create_descriptor_set(const OutputView& out) {
	if(out.size() != _size) {
		fatal("Invalid output image size.");
	}

	auto it = _descriptor_sets.find(out.vk_image_view());
	if(it == _descriptor_sets.end()) {
		it = _descriptor_sets.insert(std::make_pair(out.vk_image_view(), DescriptorSet(device(), {
				Binding(_depth),
				Binding(_diffuse),
				Binding(_normal),
				Binding(out),
				Binding(_lights)
			}))).first;
	}
	return it->second;
}

}
