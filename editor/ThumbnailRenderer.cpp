/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "ThumbnailRenderer.h"
#include "EditorResources.h"

#include <yave/assets/AssetLoader.h>
#include <yave/material/Material.h>
#include <yave/material/MaterialData.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdQueue.h>

#include <yave/renderer/DefaultRenderer.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphResourcePool.h>

#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/ecs/EntityPrefab.h>
#include <yave/ecs/EntityWorld.h>
#include <yave/scene/EcsScene.h>

#include <yave/utils/color.h>

#include <y/serde3/archives.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

static std::string_view byte_size_text(usize size) {
    if(size < 8 * 1024) {
        return fmt("{}B", size);
    }
    if(size < 8 * 1024 * 1024) {
        return fmt("{}KB", size / 1024);
    }
    return fmt("{}MB", size / (1024 * 1024));
}

static void fill_world(ecs::EntityWorld& world) {
    const ecs::EntityId sky_id = world.create_entity();
    SkyLightComponent* sky = world.get_or_add_component<SkyLightComponent>(sky_id);
    sky->probe() = device_resources().ibl_probe();
    sky->intensity() = 0.5f;
}

static AABB merge_empty_aabb(const AABB& a, const AABB& b) {
    if(a.is_empty()) {
        return b;
    }
    if(b.is_empty()) {
        return a;
    }
    return a.merged(b);
}

static AABB compute_entity_aabb(const ecs::EntityWorld& world, ecs::EntityId id) {
    AABB aabb;

    if(const StaticMeshComponent* mesh_comp = world.component<StaticMeshComponent>(id)) {
        if(const StaticMesh* mesh = mesh_comp->mesh().get()) {
            if(const TransformableComponent* trans_comp = world.component<TransformableComponent>(id)) {
                aabb = merge_empty_aabb(aabb, trans_comp->to_global(mesh->aabb()));
            }
        }
    }

    for(const ecs::EntityId child : world.children(id)) {
        aabb = merge_empty_aabb(aabb, compute_entity_aabb(world, child));
    }

    return aabb;
}

static Texture render_world(ecs::EntityWorld& world) {
    y_profile();

    AABB aabb;
    for(const ecs::EntityId id : world.entity_pool().ids()) {
        aabb = merge_empty_aabb(aabb, compute_entity_aabb(world, id));
    }

    const EcsScene scene(&world);

    const float camera_distance = std::max(math::epsilon<float>, aabb.radius()) * 2.0f;
    const Camera camera(
        math::look_at(aabb.center() + math::Vec3(0.0f, 1.0f, 1.0f) * camera_distance, aabb.center(), math::Vec3(0.0f, 1.0f, 0.0f)),
        math::perspective(math::to_rad(45.0f), 1.0f, 0.1f)
    );

    const SceneView scene_view(&scene, camera);

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    StorageTexture out = StorageTexture(ImageFormat(VK_FORMAT_R8G8B8A8_UNORM), math::Vec2ui(ThumbnailRenderer::thumbnail_size));
    {
        const auto region = recorder.region("Thumbnail cache render");

        auto resource_pool = std::make_shared<FrameGraphResourcePool>();
        FrameGraph graph(resource_pool);

        RendererSettings settings;
        {
            settings.tone_mapping.auto_exposure = false;
            settings.tone_mapping.exposure = 10.0f;
            settings.taa.enable = false;
            settings.ao.method = AOSettings::AOMethod::RTAOFallback;
        }

        const DefaultRenderer renderer = DefaultRenderer::create(graph, scene_view, out.size(), settings);

        const FrameGraphImageId output_image = renderer.tone_mapping.tone_mapped;
        {
            FrameGraphComputePassBuilder builder = graph.add_compute_pass("Thumbnail copy pass");
            builder.add_uniform_input(output_image);
            builder.add_uniform_input(renderer.gbuffer.depth);
            builder.add_external_input(StorageView(out));
            builder.set_render_func([size = out.size()](CmdBufferRecorder& rec, const FrameGraphPass* self) {
                rec.dispatch_threads(resources()[EditorResources::ThumbnailProgram], size, self->descriptor_set());
            });
        }

        graph.render(recorder);
    }
    recorder.submit().wait();

    return out;
}


static Texture render_object(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat) {
    y_profile();

    ecs::EntityWorld world;
    fill_world(world);

    *world.get_or_add_component<StaticMeshComponent>(world.create_entity()) = StaticMeshComponent(mesh, mat);

    return render_world(world);
}


static Texture render_prefab(const AssetPtr<ecs::EntityPrefab>& prefab) {
    y_profile();

    ecs::EntityWorld world;
    fill_world(world);
    world.create_entity(*prefab);

    {
        Y_TODO(we should not have to do this)
        y_profile_zone("world tick");
        world.add_system<AssetLoaderSystem>(asset_loader(), AssetLoadingFlags::SynchronousLoad);
        world.tick(job_system());
    }

    return render_world(world);
}

static Texture render_texture(const AssetPtr<Texture>& tex) {
    y_profile();

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    StorageTexture out = StorageTexture(ImageFormat(VK_FORMAT_R8G8B8A8_UNORM), math::Vec2ui(ThumbnailRenderer::thumbnail_size));
    {
        const auto descriptors = make_descriptor_set(Descriptor(*tex, SamplerType::LinearClamp), StorageView(out));
        recorder.dispatch_threads(device_resources()[DeviceResources::CopyProgram], out.size(), DescriptorSetProxy(descriptors));
    }
    recorder.submit().wait();
    return out;
}





ThumbnailRenderer::ThumbnailRenderer(AssetLoader& loader) : _loader(&loader) {
}

const ThumbnailRenderer::ThumbnailData* ThumbnailRenderer::thumbnail_data(AssetId id) {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return nullptr;
    }

    return _thumbnails.locked([&](auto&& thumbnails) -> const ThumbnailData* {
        auto& data = thumbnails[id];

        if(!data) {
            data = schedule_render(id);
            return nullptr;
        }

        if(data->status == ThumbnailStatus::Failed) {
            return nullptr;
        }

        if(data->status == ThumbnailStatus::Done) {
            return data.get();
        }

        return nullptr;
    });
}

const TextureView* ThumbnailRenderer::thumbnail_img(AssetId id) {
    const ThumbnailData* thumb = thumbnail_data(id);
    return thumb ? &thumb->view : nullptr;
}

usize ThumbnailRenderer::cached_thumbnails() {
    return _thumbnails.locked([&](auto&& thumbnails) { return thumbnails.size(); });
}

std::unique_ptr<ThumbnailRenderer::ThumbnailData> ThumbnailRenderer::schedule_render(AssetId id) {
    y_profile_zone("schedule render");

    auto data = std::make_unique<ThumbnailData>();
    job_system().schedule([this, data = data.get(), id]() {
        y_debug_assert(data->status == ThumbnailStatus::Rendering);

        const AssetType asset_type = _loader->store().asset_type(id).unwrap_or(AssetType::Unknown);

        if(const auto r = _loader->store().data(id); r.is_ok()) {
            data->infos.emplace_back(fmt("Size on disk: {}",  byte_size_text(r.unwrap()->remaining())));
        }

        switch(asset_type) {
            case AssetType::Mesh:
                if(const auto ptr = _loader->load<StaticMesh>(id)) {
                    data->texture = render_object(ptr, device_resources()[DeviceResources::EmptyMaterial]);

                    const MeshDrawData& draw_data = ptr->draw_data();
                    data->infos.emplace_back(fmt("Vertices: {}", draw_data.vertex_count()));
                    data->infos.emplace_back(fmt("Triangles: {}", draw_data.draw_command().index_count / 3));
                }
            break;

            case AssetType::Image:
                if(const auto ptr = _loader->load<Texture>(id)) {
                    data->texture = render_texture(ptr);

                    data->infos.emplace_back(fmt("Size: {}x{}", ptr->size().x(), ptr->size().y()));
                    data->infos.emplace_back(fmt("Format: {}", ptr->format().name()));
                }
            break;

            case AssetType::Material:
                if(const auto ptr = _loader->load<Material>(id)) {
                    data->texture = render_object(device_resources()[DeviceResources::SphereMesh], ptr);
                }
            break;

            case AssetType::Prefab:
                if(const auto ptr = _loader->load<ecs::EntityPrefab>(id)) {
                    data->texture = render_prefab(ptr);
                }
            break;

            default:
                log_msg(fmt("Unknown asset type {} for {}", asset_type, stringify_id(id)), Log::Error);
            break;
        }

        if(!data->texture.is_null()) {
            data->view = data->texture;
            data->status = ThumbnailStatus::Done;
        } else {
            data->status = ThumbnailStatus::Failed;
        }
    });

    return data;
}

}

