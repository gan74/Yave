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

namespace yave {

static constexpr vk::Format depth_format = vk::Format::eD32Sfloat;
static constexpr vk::Format diffuse_format = vk::Format::eR8G8B8A8Unorm;
static constexpr vk::Format normal_format = vk::Format::eR8G8B8A8Unorm;

static RenderPass create_render_pass(DevicePtr dptr) {
	return RenderPass(dptr, depth_format, {
			diffuse_format,
			normal_format
		});
}

static ComputeShader create_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("deferred.comp.spv")));
}

DeferredRenderer::DeferredRenderer(SceneView &scene, const OutputView& output) :
		DeviceLinked(scene.device()),
		_scene(scene),
		_render_pass(create_render_pass(device())),
		_depth(device(), depth_format, output.size()),
		_diffuse(device(), diffuse_format, output.size()),
		_normal(device(), normal_format, output.size()),
		_gbuffer(_render_pass, _depth, {_diffuse, _normal}),
		_output(output),
		_shader(create_shader(device())),
		_program(_shader),
		_compute_set(device(), {Binding(_depth), Binding(_diffuse), Binding(_normal)/*, Binding(_output)*/}) {
}

void DeferredRenderer::draw(CmdBufferRecorder& recorder) const {
	recorder.bind_framebuffer(_render_pass, _gbuffer);
	_scene.draw(recorder);

#warning SYNCHRONIZATION ?
	recorder.dispatch(_program, math::Vec3ui(_output.size(), 1), _compute_set);
}

}
