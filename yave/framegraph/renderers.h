/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVE_FRAMEGRAPH_RENDERERS_H
#define YAVE_FRAMEGRAPH_RENDERERS_H

#include <yave/scene/SceneView.h>
#include <yave/graphics/bindings/DescriptorSet.h>

#include "FrameGraph.h"

namespace yave {

struct SceneRenderSubPass {
	const SceneView* scene_view;

	FrameGraphMutableTypedBufferId<math::Matrix4<>> camera_buffer;
	FrameGraphMutableTypedBufferId<math::Transform<>> transform_buffer;

};

SceneRenderSubPass create_scene_render(FrameGraph& framegraph, FrameGraphPassBuilder& builder, const SceneView* view);
void render_scene(RenderPassRecorder& recorder, const SceneRenderSubPass& subpass, const FrameGraphPass* pass);



struct GBufferPass {
	SceneRenderSubPass scene_pass;

	FrameGraphImageId depth;
	FrameGraphImageId color;
	FrameGraphImageId normal;
};

GBufferPass render_gbuffer(FrameGraph& framegraph, const SceneView* view, const math::Vec2ui& size);



struct LightingPass {
	FrameGraphImageId lighting;
};

LightingPass render_lighting(FrameGraph& framegraph, const GBufferPass& gbuffer);


}

#endif // YAVE_FRAMEGRAPH_RENDERERS_H
