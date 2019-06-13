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

#include "SceneRenderSubPass.h"

#include <yave/framegraph/FrameGraph.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/entities/entities.h>


namespace yave {

static constexpr usize max_batch_size = 128 * 1024;

SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const SceneView& view) {
	auto camera_buffer = builder.declare_typed_buffer<math::Matrix4<>>();
	auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(max_batch_size);

	SceneRenderSubPass pass;
	pass.scene_view = view;
	pass.camera_buffer = camera_buffer;
	pass.transform_buffer = transform_buffer;

	builder.add_uniform_input(camera_buffer);
	builder.add_attrib_input(transform_buffer);
	builder.map_update(camera_buffer);
	builder.map_update(transform_buffer);

	return pass;
}

static usize render_world(const SceneRenderSubPass* sub_pass, RenderPassRecorder& recorder, const FrameGraphPass* pass, usize index = 0) {
	y_profile();

	const ecs::EntityWorld& world = sub_pass->scene_view.world();

	auto transform_mapping = pass->resources()->mapped_buffer(sub_pass->transform_buffer);
	auto transforms = pass->resources()->buffer<BufferUsage::AttributeBit>(sub_pass->transform_buffer);
	const auto& descriptor_set = pass->descriptor_sets()[0];

	recorder.bind_attrib_buffers({transforms, transforms});

	for(const auto& [tr, me] : world.view(StaticMeshArchetype())) {
		transform_mapping[index] = tr.transform();
		me.render(recorder, Renderable::SceneData{descriptor_set, u32(index)});
		++index;
	}

	return index;
}

void SceneRenderSubPass::render(RenderPassRecorder& recorder, const FrameGraphPass* pass) const {
	y_profile();

	// fill render data
	auto camera_mapping = pass->resources()->mapped_buffer(camera_buffer);
	camera_mapping[0] = scene_view.camera().viewproj_matrix();

	usize index = 0;
	if(scene_view.has_world()) {
		index = render_world(this, recorder, pass, index);
	}
}

}
