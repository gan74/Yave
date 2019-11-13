/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/entities/entities.h>
#include <yave/utils/color.h>

#include <editor/utils/assets.h>

#include <thread>

namespace editor {

static math::Transform<> center_to_camera(const AABB& box) {
	float scale = 0.22f / std::max(math::epsilon<float>, box.radius());
	float angle = (box.extent().x() > box.extent().y() ? 90.0f : 0.0f) + 30.0f;
	auto rot = math::Quaternion<>::from_euler(0.0f, math::to_rad(angle), 0.0f);
	math::Vec3 tr = rot(box.center() * scale);
	return math::Transform<>(math::Vec3(0.0f, -0.65f, 0.65f) - tr,
							 rot,
							 math::Vec3(scale));
}

ThumbmailCache::ThumbmailData::ThumbmailData(DevicePtr dptr, usize size, AssetId asset) :
		image(dptr, vk::Format::eR8G8B8A8Unorm, math::Vec2ui(size)),
		view(image),
		id(asset) {
}

ThumbmailCache::SceneData::SceneData(ContextPtr ctx, const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat)
		: view(&world) {

	DevicePtr dptr = ctx->device();
	float intensity = 0.35f;

	{
		ecs::EntityId light_id = world.create_entity(DirectionalLightArchetype());
		DirectionalLightComponent* light_comp = world.component<DirectionalLightComponent>(light_id);
		light_comp->direction() = math::Vec3{0.0f, 0.3f, -1.0f};
		light_comp->intensity() = 3.0f * intensity;
	}
	{
		ecs::EntityId light_id = world.create_entity(PointLightArchetype());
		world.component<TransformableComponent>(light_id)->position() = math::Vec3(0.75f, -0.5f, 0.5f);
		PointLightComponent* light = world.component<PointLightComponent>(light_id);
		light->color() = k_to_rbg(2500.0f);
		light->intensity() = 1.5f * intensity;
		light->falloff() = 0.5f;
		light->radius() = 2.0f;
	}
	{
		ecs::EntityId light_id = world.create_entity(PointLightArchetype());
		world.component<TransformableComponent>(light_id)->position() = math::Vec3(-0.75f, -0.5f, 0.5f);
		PointLightComponent* light = world.component<PointLightComponent>(light_id);
		light->color() = k_to_rbg(10000.0f);
		light->intensity() = 1.5f * intensity;
		light->falloff() = 0.5f;
		light->radius() = 2.0f;
	}

	{
		ecs::EntityId bg_id = world.create_entity(StaticMeshArchetype());
		StaticMeshComponent* mesh_comp = world.component<StaticMeshComponent>(bg_id);
		*mesh_comp = StaticMeshComponent(dptr->device_resources()[DeviceResources::SweepMesh], dptr->device_resources()[DeviceResources::EmptyMaterial]);
	}

	{
		ecs::EntityId mesh_id = world.create_entity(StaticMeshArchetype());
		StaticMeshComponent* mesh_comp = world.component<StaticMeshComponent>(mesh_id);
		*mesh_comp = StaticMeshComponent(mesh, mat);

		TransformableComponent* trans_comp = world.component<TransformableComponent>(mesh_id);
		trans_comp->transform() = center_to_camera(mesh->aabb());

		y_debug_assert(world.component<StaticMeshComponent>(mesh_id)->material()->mat_template());
	}

	view.camera().set_view(math::look_at(math::Vec3(0.0f, -1.0f, 1.0f), math::Vec3(0.0f), math::Vec3(0.0f, 0.0f, 1.0f)));
}


static auto load_envmap() {
	const math::Vec4 data(0.1f);
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

ThumbmailCache::Thumbmail ThumbmailCache::get_thumbmail(AssetId asset) {
	if(asset == AssetId::invalid_id()) {
		return Thumbmail{};
	}

	process_requests();
	if(auto it = _thumbmails.find(asset); it != _thumbmails.end()) {
		if(it->second) {
			return Thumbmail{&it->second->view, it->second->properties};
		} else {
			return Thumbmail{};
		}
	}
	request_thumbmail(asset);
	return Thumbmail{};
}

void ThumbmailCache::process_requests() {
	std::unique_ptr<CmdBufferRecorder> recorder;

	static constexpr usize max_parallel_requests = 8;
	for(usize i = 0; i != std::min(max_parallel_requests, _requests.size()); ++i) {
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

			AssetType asset_type = context()->asset_store().asset_type(id).unwrap_or(AssetType::Unknown);
			switch(asset_type) {
				case AssetType::Mesh:
					if(auto mesh = context()->loader().load<StaticMesh>(id)) {
						return [=, m = std::move(mesh.unwrap())](CmdBufferRecorder& rec) {
								return render_thumbmail(rec, id, m, device()->device_resources()[DeviceResources::EmptyMaterial]);
							};
					}
				break;

				case AssetType::Material:
					if(auto mat = context()->loader().load<Material>(id)) {
						return [=, m = std::move(mat.unwrap())](CmdBufferRecorder& rec) {
								return render_thumbmail(rec, id, rec.device()->device_resources()[DeviceResources::SphereMesh], m);
							};
					}
				break;

				case AssetType::Image:
					if(auto tex = context()->loader().load<Texture>(id)) {
						return [=, t = std::move(tex.unwrap())](CmdBufferRecorder& rec) { return render_thumbmail(rec, t); };
					}
				break;

				default:
				break;
			}
			log_msg(fmt("Unable to load %.", asset_type_name(asset_type)), Log::Error);


			return [](CmdBufferRecorder&) { return nullptr; };
		});
}

static std::array<char, 32> rounded_string(float value) {
	std::array<char, 32> buffer;
	std::snprintf(buffer.data(), buffer.size(), "%.2f", double(value));
	return buffer;
}

static void add_size_property(core::Vector<std::pair<core::String, core::String>>& properties, ContextPtr ctx, AssetId id) {
	std::array suffixes = {"B", "KB", "MB", "GB"};
	if(auto data = ctx->asset_store().data(id)) {
		float size = data.unwrap()->remaining();
		usize i = 0;
		for(; i != suffixes.size() - 1; ++i) {
			if(size < 1024.0f) {
				break;
			}
			size /= 1024.0f;
		}
		properties.emplace_back("Size on disk", fmt("% %", rounded_string(size).data(), suffixes[i]));
	}
}

std::unique_ptr<ThumbmailCache::ThumbmailData> ThumbmailCache::render_thumbmail(CmdBufferRecorder& recorder, const AssetPtr<Texture>& tex) const {
	auto thumbmail = std::make_unique<ThumbmailData>(device(), _size, tex.id());

	{
		DescriptorSet set(device(), {Descriptor(*tex), Descriptor(StorageView(thumbmail->image))});
		recorder.dispatch_size(device()->device_resources()[DeviceResources::CopyProgram],  math::Vec2ui(_size), {set});
	}

	thumbmail->properties.emplace_back("Size", fmt("% x %", tex->size().x(), tex->size().y()));
	thumbmail->properties.emplace_back("Mipmaps", fmt("%", tex->mipmaps()));
	thumbmail->properties.emplace_back("Format", tex->format().name());
	add_size_property(thumbmail->properties, context(), tex.id());

	return thumbmail;
}

std::unique_ptr<ThumbmailCache::ThumbmailData> ThumbmailCache::render_thumbmail(CmdBufferRecorder& recorder, AssetId id, const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat) const {
	auto thumbmail = std::make_unique<ThumbmailData>(device(), _size, id);
	SceneData scene(context(), mesh, mat);

	{
		auto region = recorder.region("Thumbmail cache render");

		FrameGraph graph(context()->resource_pool());
		RendererSettings settings;
		settings.tone_mapping.auto_exposure = false;
		DefaultRenderer renderer = DefaultRenderer::create(graph, scene.view, thumbmail->image.size(), _ibl_data, settings);

		FrameGraphImageId output_image = renderer.tone_mapping.tone_mapped;
		{
			FrameGraphPassBuilder builder = graph.add_pass("Thumbmail copy pass");
			builder.add_uniform_input(output_image);
			builder.add_uniform_input(renderer.gbuffer.depth);
			builder.add_uniform_input(StorageView(thumbmail->image));
			builder.set_render_func([=](CmdBufferRecorder& rec, const FrameGraphPass* self) {
					rec.dispatch_size(context()->resources()[EditorResources::DepthAlphaProgram], math::Vec2ui(_size), {self->descriptor_sets()[0]});
				});
		}

		std::move(graph).render(recorder);
	}

	if(id == mesh.id()) {
		thumbmail->properties.emplace_back("Triangles", fmt("%", mesh->triangle_buffer().size()));
		thumbmail->properties.emplace_back("Radius", fmt("%", rounded_string(mesh->radius()).data()));
	}
	add_size_property(thumbmail->properties, context(), id);

	return thumbmail;
}


}
