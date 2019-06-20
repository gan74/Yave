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

#include <editor/renderer/PickingPass.h>

namespace editor {

PickingManager::PickingManager(ContextPtr ctx) :
		ContextLinked(ctx),
		_buffer(device(), 1) {
}


PickingManager::PickingData PickingManager::pick_sync(const math::Vec2& uv, const math::Vec2ui& size) {
	y_profile();

	FrameGraph framegraph(context()->resource_pool());


	PickingPass scene_pass = PickingPass::create(context(), framegraph, context()->scene_view(), size);

	{
		FrameGraphPassBuilder builder = framegraph.add_pass("Picking readback pass");

		builder.add_uniform_input(scene_pass.depth);
		builder.add_uniform_input(scene_pass.id);
		builder.add_descriptor_binding(Binding(_buffer));

		builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			const auto& program = context()->resources()[EditorResources::PickingProgram];
			recorder.dispatch(program, math::Vec3ui(1), {self->descriptor_sets()[0]}, uv);
		});
	}

	CmdBufferRecorder recorder = device()->create_disposable_cmd_buffer();
	std::move(framegraph).render(recorder);
	device()->graphic_queue().submit<SyncSubmit>(std::move(recorder));

	ReadBackData read_back = TypedMapping(_buffer)[0];
	float depth = read_back.depth;

	auto inv_matrix = context()->scene_view().camera().inverse_matrix();
	math::Vec4 p = inv_matrix * math::Vec4(uv * 2.0f - 1.0f, depth, 1.0f);

	PickingData data{
			p.to<3>() / p.w(),
			depth,
			uv,
			read_back.id
		};

	log_msg(fmt("picked: % (id: %)", data.world_pos, data.instance_id));
	return data;
}

}
