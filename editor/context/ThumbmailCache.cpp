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

#include <yave/renderers/TiledDeferredRenderer.h>
#include <yave/renderers/FramebufferRenderer.h>
#include <yave/renderers/ToneMapper.h>

namespace editor {


ThumbmailCache::Thumbmail::Thumbmail(DevicePtr dptr, usize size) :
	image(dptr, vk::Format::eR8G8B8A8Unorm, math::Vec2ui(size)),
	view(image) {
}

ThumbmailCache::ThumbmailCache(ContextPtr ctx, usize size) :
	ContextLinked(ctx),
	_size(size),
	_scene_view(_scene) {

	create_renderer();
}

math::Vec2ui ThumbmailCache::thumbmail_size() const {
	return math::Vec2ui(_size);
}

void ThumbmailCache::create_renderer() {
	auto ibl		= Node::Ptr<IBLData>(new IBLData(device()));
	auto scene		= Node::Ptr<SceneRenderer>(new SceneRenderer(device(), _scene_view));
	auto gbuffer	= Node::Ptr<GBufferRenderer>(new GBufferRenderer(scene, thumbmail_size()));
	auto deferred	= Node::Ptr<TiledDeferredRenderer>(new TiledDeferredRenderer(gbuffer, ibl));
	auto tonemap	= Node::Ptr<SecondaryRenderer>(new ToneMapper(deferred));
	_renderer		= Node::Ptr<FramebufferRenderer>(new FramebufferRenderer(tonemap, thumbmail_size()));
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

void ThumbmailCache::render_thumbmail(const AssetPtr<StaticMesh>& mesh) {

	auto thumb = std::make_unique<Thumbmail>(device(), _size);

	CmdBufferRecorder rec = (device()->create_disposable_cmd_buffer());
	RenderingPipeline pipe(_renderer);
	rec.blit(_renderer->output_image(), thumb->image);
	pipe.render(rec, FrameToken::create_disposable(thumb->image));

	/*rec.keep_alive(ScopeExit([this, mesh, th = std::move(thumb)]() mutable { _thumbmails[mesh.id()] = std::move(thumb); }));
	device()->queue(vk::QueueFlagBits::eGraphics).submit<AsyncSubmit>(std::move(rec));*/

	device()->queue(vk::QueueFlagBits::eGraphics).submit<SyncSubmit>(std::move(rec));
	_thumbmails[mesh.id()] = std::move(thumb);
}


}
