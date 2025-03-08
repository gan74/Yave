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

#include "ThumbmailRenderer.h"
#include "EditorResources.h"

#include <yave/assets/AssetLoader.h>
#include <yave/material/Material.h>
#include <yave/material/MaterialData.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/descriptors/DescriptorSet.h>

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

static Texture render_scene(const Scene& scene) {
    y_profile();

    const Camera camera(
        math::look_at(math::Vec3(0.0f, -1.0f, 1.0f), math::Vec3(0.0f), math::Vec3(0.0f, 0.0f, 1.0f)),
        math::perspective(math::to_rad(45.0f), 1.0f, 0.1f)
    );

    const SceneView scene_view(&scene, camera);

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    StorageTexture out = StorageTexture(ImageFormat(VK_FORMAT_R8G8B8A8_UNORM), math::Vec2ui(ThumbmailRenderer::thumbmail_size));
    {
        const auto region = recorder.region("Thumbmail cache render");

        auto resource_pool = std::make_shared<FrameGraphResourcePool>();
        FrameGraph graph(resource_pool);

        RendererSettings settings;
        {
            settings.tone_mapping.auto_exposure = false;
            settings.taa.enable = false;
        }

        const DefaultRenderer renderer = DefaultRenderer::create(graph, scene_view, out.size(), settings);

        const FrameGraphImageId output_image = renderer.tone_mapping.tone_mapped;
        {
            FrameGraphComputePassBuilder builder = graph.add_compute_pass("Thumbmail copy pass");
            builder.add_uniform_input(output_image);
            builder.add_uniform_input(renderer.gbuffer.depth);
            builder.add_external_input(StorageView(out));
            builder.set_render_func([size = out.size()](CmdBufferRecorder& rec, const FrameGraphPass* self) {
                rec.dispatch_threads(resources()[EditorResources::DepthAlphaProgram], size, self->descriptor_set());
            });
        }

        graph.render(recorder);
    }
    recorder.submit().wait();

    return out;
}

static void fill_world(ecs::EntityWorld& world) {
    const float intensity = 0.25f;

    {
        const ecs::EntityId light_id = world.create_entity();
        DirectionalLightComponent* light_comp = world.get_or_add_component<DirectionalLightComponent>(light_id);
        light_comp->direction() = math::Vec3{0.0f, 0.3f, -1.0f};
        light_comp->intensity() = 3.0f * intensity;
    }
    {
        const ecs::EntityId light_id = world.create_entity();
        world.get_or_add_component<TransformableComponent>(light_id)->set_position(math::Vec3(0.75f, -0.5f, 0.5f));
        PointLightComponent* light = world.get_or_add_component<PointLightComponent>(light_id);
        light->color() = k_to_rbg(2500.0f);
        light->intensity() = 1.5f * intensity;
        light->falloff() = 0.5f;
        light->range() = 2.0f;
    }
    {
        const ecs::EntityId light_id = world.create_entity();
        world.get_or_add_component<TransformableComponent>(light_id)->set_position(math::Vec3(-0.75f, -0.5f, 0.5f));
        PointLightComponent* light = world.get_or_add_component<PointLightComponent>(light_id);
        light->color() = k_to_rbg(10000.0f);
        light->intensity() = 1.5f * intensity;
        light->falloff() = 0.5f;
        light->range() = 2.0f;
    }

    {
        const ecs::EntityId sky_id = world.create_entity();
        SkyLightComponent* sky = world.get_or_add_component<SkyLightComponent>(sky_id);
        sky->probe() = device_resources().ibl_probe();
        sky->intensity() = intensity;
    }

}

static math::Transform<> center_to_camera(const AABB& box) {
    const float scale = 0.25f / std::max(math::epsilon<float>, box.radius());
    const float angle = (box.extent().x() > box.extent().y() ? 90.0f : 0.0f) + 30.0f;
    const auto rot = math::Quaternion<>::from_euler(0.0f, math::to_rad(angle), 0.0f);
    const math::Vec3 tr = rot(box.center() * scale);
    return math::Transform<>(math::Vec3(0.0f, -0.65f, 0.65f) - tr,
                             rot,
                             math::Vec3(scale));
}

static Texture render_object(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat) {
    y_profile();

    ecs::EntityWorld world;
    fill_world(world);

    {
        const ecs::EntityId entity = world.create_entity();
        *world.get_or_add_component<StaticMeshComponent>(entity) = StaticMeshComponent(mesh, mat);
        world.get_or_add_component<TransformableComponent>(entity)->set_transform(center_to_camera(mesh->aabb()));
    }

    return render_scene(EcsScene(&world));
}

static Texture render_prefab(const AssetPtr<ecs::EntityPrefab>& prefab) {
    y_profile();

    ecs::EntityWorld world;
    fill_world(world);

    {
        const ecs::EntityId entity = world.create_entity(*prefab);
        if(const StaticMeshComponent* mesh_comp = world.component<StaticMeshComponent>(entity)) {
            if(const StaticMesh* mesh = mesh_comp->mesh().get()) {
                if(TransformableComponent* trans_comp = world.component_mut<TransformableComponent>(entity)) {
                    trans_comp->set_transform(center_to_camera(mesh->aabb()));
                }
            }
        }
    }

    return render_scene(EcsScene(&world));
}

static Texture render_texture(const AssetPtr<Texture>& tex) {
    y_profile();

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    StorageTexture out = StorageTexture(ImageFormat(VK_FORMAT_R8G8B8A8_UNORM), math::Vec2ui(ThumbmailRenderer::thumbmail_size));
    {
        const auto ds = DescriptorSet(Descriptor(*tex, SamplerType::LinearClamp), StorageView(out));
        recorder.dispatch_threads(device_resources()[DeviceResources::CopyProgram],  out.size(), ds);
    }
    recorder.submit().wait();
    return out;
}





ThumbmailRenderer::ThumbmailRenderer(AssetLoader& loader) : _loader(&loader) {
}

const TextureView* ThumbmailRenderer::thumbmail(AssetId id) {
    y_profile();

    if(id == AssetId::invalid_id()) {
        return nullptr;
    }

    return _thumbmails.locked([&](auto&& thumbmails) -> const TextureView* {
        auto& data = thumbmails[id];

        if(!data) {
            data = schedule_render(id);
            return nullptr;
        }

        if(data->failed) {
            return nullptr;
        }

        if(data->done.is_ready()) {
            return &data->view;
        }

        return nullptr;
    });
}

usize ThumbmailRenderer::cached_thumbmails() {
    return _thumbmails.locked([&](auto&& thumbmails) { return thumbmails.size(); });
}

std::unique_ptr<ThumbmailRenderer::ThumbmailData> ThumbmailRenderer::schedule_render(AssetId id) {
    y_profile_zone("schedule render");

    auto data = std::make_unique<ThumbmailData>();
    _render_thread.schedule([this, data = data.get(), id]() {
        const AssetType asset_type = _loader->store().asset_type(id).unwrap_or(AssetType::Unknown);
        data->failed = true;
        switch(asset_type) {
            case AssetType::Mesh:
                if(const auto ptr = _loader->load<StaticMesh>(id)) {
                    data->failed = false;
                    data->asset_ptr = ptr;
                    data->texture = render_object(ptr, device_resources()[DeviceResources::EmptyMaterial]);
                }
            break;

            case AssetType::Image:
                if(const auto ptr = _loader->load<Texture>(id)) {
                    data->failed = false;
                    data->asset_ptr = ptr;
                    data->texture = render_texture(ptr);
                }
            break;

            case AssetType::Material:
                if(const auto ptr = _loader->load<Material>(id)) {
                    data->failed = false;
                    data->asset_ptr = ptr;
                    data->texture = render_object(device_resources()[DeviceResources::SphereMesh], ptr);
                }
            break;

            case AssetType::Prefab:
                if(const auto ptr = _loader->load<ecs::EntityPrefab>(id)) {
                    data->failed = false;
                    data->asset_ptr = ptr;
                    data->texture = render_prefab(ptr);
                }
            break;

            default:
                log_msg(fmt("Unknown asset type {} for {}", asset_type, stringify_id(id)), Log::Error);
            break;
        }

        if(!data->failed) {
            data->failed = data->texture.is_null();
        }

        if(!data->failed) {
            data->view = data->texture;
        }

    }, &data->done);

    return data;
}

}

