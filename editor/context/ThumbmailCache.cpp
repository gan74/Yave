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

#include "ThumbmailCache.h"

#include <editor/context/EditorContext.h>
#include <yave/renderer/renderer.h>

#include <thread>

namespace editor {

ThumbmailCache::Thumbmail::Thumbmail(DevicePtr dptr, usize size, AssetId asset) :
		image(dptr, vk::Format::eR8G8B8A8Unorm, math::Vec2ui(size)),
		view(image),
		id(asset) {
}

ThumbmailCache::SceneData::SceneData(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat)
		: view(&scene) {

	Light light(Light::Directional);
	light.transform().set_basis(math::Vec3{1.0f, 0.5f, -1.0f}.normalized(), {1.0f, 0.0f, 0.0f});
	light.color() = 5.0f;
	scene.lights() << std::make_unique<Light>(light);
	scene.static_meshes() << std::make_unique<StaticMeshInstance>(mesh, mat);
	view.camera().set_view(math::look_at(math::Vec3(mesh->radius() * 1.5f), math::Vec3(0.0f), math::Vec3(0.0f, 0.0f, 1.0f)));
}


static auto load_envmap() {
	math::Vec4 data(1.0f, 1.0f, 1.0f, 1.0f);
	return ImageData(math::Vec2ui(1), reinterpret_cast<const u8*>(data.data()), vk::Format::eR32G32B32A32Sfloat);
}

ThumbmailCache::ThumbmailCache(ContextPtr ctx, usize size) :
		ContextLinked(ctx),
		_size(size),
		_ibl_data(std::make_shared<IBLData>(device(), load_envmap())) {
}

void ThumbmailCache::clear() {
	_thumbmails.clear();
}

math::Vec2ui ThumbmailCache::thumbmail_size() const {
	return math::Vec2ui(_size);
}

TextureView* ThumbmailCache::get_thumbmail(AssetId asset) {
	if(asset == AssetId::invalid_id()) {
		return nullptr;
	}

	process_requests();
	if(auto it = _thumbmails.find(asset); it != _thumbmails.end()) {
		if(it->second) {
			return &it->second->view;
		} else {
			return nullptr;
		}
	}
	request_thumbmail(asset);
	return nullptr;
}

void ThumbmailCache::process_requests() {
	std::unique_ptr<CmdBufferRecorder> recorder;
	for(usize i = 0; i != _requests.size(); ++i) {
		if(_requests[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			if(!recorder) {
				recorder = std::make_unique<CmdBufferRecorder>(device()->create_disposable_cmd_buffer());
			}

			ThumbmailFunc func = _requests[i].get();
			if(auto thumb = func(*recorder)) {
				_thumbmails[thumb->id] = std::move(thumb);
			}

			_requests.erase_unordered(_requests.begin() + i);
			--i;
		}
	}
	if(recorder) {
		device()->graphic_queue().submit<SyncSubmit>(std::move(*recorder));
	}
}

void ThumbmailCache::request_thumbmail(AssetId id) {
	_thumbmails[id] = nullptr;
	_requests << std::async(std::launch::async, [=]() -> ThumbmailFunc {
			y_profile();

			switch(context()->asset_store().asset_type(id).unwrap_or(AssetType::Unknown)) {
				case AssetType::Mesh:
					if(auto mesh = context()->loader().load<StaticMesh>(id)) {
						return [=, m = std::move(mesh.unwrap())](CmdBufferRecorder& rec) {
								return render_thumbmail(rec, id, m, device()->device_resources()[DeviceResources::EmptyMaterial]);
							};
					} else {
						log_msg("Unable to load static mesh.", Log::Error);
					}
				break;

				case AssetType::Material:
					if(auto mat = context()->loader().load<Material>(id)) {
						return [=, m = std::move(mat.unwrap())](CmdBufferRecorder& rec) {
								return render_thumbmail(rec, id, rec.device()->device_resources()[DeviceResources::SphereMesh], m);
							};
					} else {
						log_msg("Unable to load material.", Log::Error);
					}
				break;

				case AssetType::Image:
					if(auto tex = context()->loader().load<Texture>(id)) {
						return [=, t = std::move(tex.unwrap())](CmdBufferRecorder& rec) { return render_thumbmail(rec, t); };
					} else {
						log_msg("Unable to load image.", Log::Error);
					}
				break;

				default:
				break;
			}

			return [](CmdBufferRecorder&) { return nullptr; };
		});
}

std::unique_ptr<ThumbmailCache::Thumbmail> ThumbmailCache::render_thumbmail(CmdBufferRecorder& recorder, const AssetPtr<Texture>& tex) const {
	auto thumbmail = std::make_unique<Thumbmail>(device(), _size, tex.id());

	{
		DescriptorSet set(device(), {Binding(*tex), Binding(StorageView(thumbmail->image))});
		recorder.dispatch_size(device()->device_resources()[DeviceResources::CopyProgram],  math::Vec2ui(_size), {set});
	}

	return thumbmail;
}

std::unique_ptr<ThumbmailCache::Thumbmail> ThumbmailCache::render_thumbmail(CmdBufferRecorder& recorder, AssetId id, const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat) const {
	auto thumbmail = std::make_unique<Thumbmail>(device(), _size, id);
	SceneData scene(mesh, mat);

	{
		auto region = recorder.region("Thumbmail cache render");

		FrameGraph graph(context()->resource_pool());
		DefaultRenderer renderer = DefaultRenderer::create(graph, scene.view, thumbmail->image.size(), _ibl_data);

		FrameGraphImageId output_image = renderer.tone_mapping.tone_mapped;
		{
			FrameGraphPassBuilder builder = graph.add_pass("Thumbmail copy pass");
			builder.add_uniform_input(output_image);
			builder.add_uniform_input(renderer.gbuffer.depth);
			builder.add_uniform_input(StorageView(thumbmail->image));
			builder.set_render_func([=](CmdBufferRecorder& rec, const FrameGraphPass* self) {
					rec.dispatch_size(device()->device_resources()[DeviceResources::DepthAlphaProgram], math::Vec2ui(_size), {self->descriptor_sets()[0]});
				});
		}

		std::move(graph).render(recorder);
	}

	return thumbmail;
}


}
