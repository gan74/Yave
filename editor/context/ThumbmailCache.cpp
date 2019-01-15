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

#include "ThumbmailCache.h"

#include <editor/context/EditorContext.h>

#include <yave/scene/SceneView.h>
#include <yave/renderer/ToneMappingPass.h>

namespace editor {

ThumbmailCache::Thumbmail::Thumbmail(DevicePtr dptr, usize size) :
	image(dptr, vk::Format::eR8G8B8A8Unorm, math::Vec2ui(size)),
	view(image) {
}

ThumbmailCache::ThumbmailCache(ContextPtr ctx, usize size) :
	ContextLinked(ctx),
	_size(size),
	_scene_view(_scene) {
}

math::Vec2ui ThumbmailCache::thumbmail_size() const {
	return math::Vec2ui(_size);
}

TextureView* ThumbmailCache::get_thumbmail(const AssetPtr<StaticMesh>& mesh) {
	if(auto it = _thumbmails.find(mesh.id()); it != _thumbmails.end()) {
		if(it->second) {
			return &it->second->view;
		}
	} else {
		render_thumbmail(mesh);
	}
	return nullptr;
}

void ThumbmailCache::render(Thumbmail* out) {
	FrameGraph graph(context()->resource_pool());
	auto gbuffer = render_gbuffer(graph, &_scene_view, out->image.size());
	auto lighting = render_lighting(graph, gbuffer, nullptr);
	auto tone_mapping = render_tone_mapping(graph, lighting);

	/*FrameGraphImageId output_image = tone_mapping.tone_mapped;
	{
		FrameGraphPassBuilder builder = graph.add_pass("ImGui texture pass");
		builder.add_texture_input(output_image, PipelineStage::FragmentBit);
		builder.set_render_func([&output, output_image](CmdBufferRecorder& rec, const FrameGraphPass* pass) {
				auto out = std::make_unique<TextureView>(pass->resources()->image<ImageUsage::TextureBit>(output_image));
				output = out.get();
				rec.keep_alive(std::move(out));
			});
	}

	std::move(graph).render(recorder);
	recorder.keep_alive(_ibl_data);*/
}

void ThumbmailCache::render_thumbmail(const AssetPtr<StaticMesh>& mesh) {
#if 0
	auto thumb = std::make_unique<Thumbmail>(device(), _size);

	CmdBufferRecorder rec = (device()->create_disposable_cmd_buffer());
	RenderingPipeline pipe(_renderer);
	rec.blit(_renderer->output_image(), thumb->image);
	pipe.render(rec, FrameToken::create_disposable(thumb->image));

	/*rec.keep_alive(ScopeExit([this, mesh, th = std::move(thumb)]() mutable { _thumbmails[mesh.id()] = std::move(thumb); }));
	device()->queue(vk::QueueFlagBits::eGraphics).submit<AsyncSubmit>(std::move(rec));*/

	device()->queue(vk::QueueFlagBits::eGraphics).submit<SyncSubmit>(std::move(rec));
	_thumbmails[mesh.id()] = std::move(thumb);
#endif
}


}
