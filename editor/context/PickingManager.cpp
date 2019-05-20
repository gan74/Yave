/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "PickingManager.h"
#include "EditorContext.h"

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/renderer/SceneRenderSubPass.h>

namespace editor {

static constexpr usize picking_resolution = 512;

PickingManager::PickingManager(ContextPtr ctx) :
		ContextLinked(ctx),
		_buffer(device(), 1),
		_depth(device(), vk::Format::eD32Sfloat, math::Vec2ui(picking_resolution)),
		_descriptor_set(device(), {Binding(TextureView(_depth)), Binding(_buffer)}),
		_framebuffer(device(), _depth) {
}


PickingManager::PickingData PickingManager::pick_sync(const math::Vec2& uv) {
	y_profile();

	FrameGraph framegraph(context()->resource_pool());

	FrameGraphPassBuilder builder = framegraph.add_pass("Picking pass");
	SceneRenderSubPass scene_pass = SceneRenderSubPass::create(framegraph, builder, context()->scene_view());
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			{
				auto render_pass = recorder.bind_framebuffer(_framebuffer);
				scene_pass.render(render_pass, self);
			}
			const auto& program = recorder.device()->device_resources()[DeviceResources::PickingProgram];
			recorder.dispatch(program, math::Vec3ui(1), {_descriptor_set}, uv);
		});

	CmdBufferRecorder recorder = device()->create_disposable_cmd_buffer();
	std::move(framegraph).render(recorder);
	device()->graphic_queue().submit<SyncSubmit>(std::move(recorder));

	float depth = TypedMapping(_buffer)[0];

	auto inv_matrix = context()->scene_view().camera().inverse_matrix();
	math::Vec4 p = inv_matrix * math::Vec4(uv * 2.0f - 1.0f, depth, 1.0f);

	PickingData data{
			p.to<3>() / p.w(),
			depth,
			uv
		};

	log_msg(fmt("picked %", data.world_pos));
	return data;
}

}
