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

ThumbmailCache::SceneData::SceneData(DevicePtr dptr, const AssetPtr<StaticMesh>& mesh)
	: view(scene) {

	scene.static_meshes() << std::make_unique<StaticMeshInstance>(mesh, dptr->default_resources()[DefaultResources::BasicMaterial]);
	view.camera().set_view(math::look_at(math::Vec3(mesh->radius() * 1.5f), math::Vec3(0.0f), math::Vec3(0.0f, 0.0f, 1.0f)));
}


ThumbmailCache::ThumbmailCache(ContextPtr ctx, usize size) :
	ContextLinked(ctx),
	_size(size),
	_ibl_data(std::make_shared<IBLData>(device())) {
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

void ThumbmailCache::render(CmdBufferRecorder& recorder, const SceneData& scene, Thumbmail* out) {
	FrameGraph graph(context()->resource_pool());
	auto gbuffer = render_gbuffer(graph, &scene.view, out->image.size());
	auto lighting = render_lighting(graph, gbuffer, _ibl_data);
	auto tone_mapping = render_tone_mapping(graph, lighting);

	FrameGraphImageId output_image = tone_mapping.tone_mapped;
	{
		FrameGraphPassBuilder builder = graph.add_pass("Thumbmail copy pass");
		builder.add_copy_src(output_image);
		builder.set_render_func([out, output_image](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
				recorder.copy(self->resources()->image<ImageUsage::TransferSrcBit>(output_image), out->image);
			});
	}

	std::move(graph).render(recorder);
	//recorder.keep_alive(_ibl_data);
}

void ThumbmailCache::render_thumbmail(const AssetPtr<StaticMesh>& mesh) {
	auto thumbmail = std::make_unique<Thumbmail>(device(), _size);
	SceneData scene(device(), mesh);

	CmdBufferRecorder recorder = device()->create_disposable_cmd_buffer();
	render(recorder, scene, thumbmail.get());
	device()->graphic_queue().submit<SyncSubmit>(std::move(recorder));

	_thumbmails[mesh.id()] = std::move(thumbmail);
}


}
