/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <editor/utils/assets.h>
#include <editor/utils/entities.h>

#include <yave/renderer/DefaultRenderer.h>
#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphResourcePool.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/Material.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>

#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/entities/entities.h>

#include <yave/assets/AssetStore.h>
#include <yave/assets/AssetLoader.h>

#include <yave/utils/color.h>
#include <yave/utils/entities.h>


#include <thread>

namespace editor {

static math::Transform<> center_to_camera(const AABB& box) {
    const float scale = 0.22f / std::max(math::epsilon<float>, box.radius());
    const float angle = (box.extent().x() > box.extent().y() ? 90.0f : 0.0f) + 30.0f;
    const auto rot = math::Quaternion<>::from_euler(0.0f, math::to_rad(angle), 0.0f);
    const math::Vec3 tr = rot(box.center() * scale);
    return math::Transform<>(math::Vec3(0.0f, -0.65f, 0.65f) - tr,
                             rot,
                             math::Vec3(scale));
}

static std::array<char, 32> rounded_string(float value) {
    std::array<char, 32> buffer;
    std::snprintf(buffer.data(), buffer.size(), "%.2f", double(value));
    return buffer;
}

static void add_size_property(core::Vector<std::pair<core::String, core::String>>& properties, ContextPtr ctx, AssetId id) {
    const std::array suffixes = {"B", "KB", "MB", "GB"};
    if(const auto data = ctx->asset_store().data(id)) {
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





struct ThumbmailData {
    ThumbmailData(ContextPtr ctx, usize size, AssetId asset) :
            image(ctx->device(), VK_FORMAT_R8G8B8A8_UNORM, math::Vec2ui(size)),
            view(image),
            id(asset) {

        switch(ctx->asset_store().asset_type(id).unwrap_or(AssetType::Unknown)) {
            case AssetType::Mesh:
                if(const auto& mesh = ctx->loader().load<StaticMesh>(id); mesh) {
                    properties.emplace_back("Triangles", fmt("%", mesh->triangle_buffer().size()));
                    properties.emplace_back("Vertices", fmt("%", mesh->vertex_buffer().size()));
                    properties.emplace_back("Radius", fmt("%", rounded_string(mesh->radius()).data()));
                }
            break;

            case AssetType::Image:
                if(const auto& tex = ctx->loader().load<Texture>(id); tex) {
                    properties.emplace_back("Size", fmt("% x %", tex->size().x(), tex->size().y()));
                    properties.emplace_back("Mipmaps", fmt("%", tex->mipmaps()));
                    properties.emplace_back("Format", tex->format().name());
                }
            break;

            case AssetType::Prefab:
                if(const auto& prefab = ctx->loader().load<ecs::EntityPrefab>(id); prefab) {
                    properties.emplace_back("Components", fmt("%", prefab->components().size()));
                    for(usize i = 0; i != prefab->components().size(); ++i) {
                        if(const auto* component = prefab->components()[i].get()) {
                            properties.emplace_back(core::String(fmt("%", i)), clean_component_name(component->runtime_info().type_name));
                        }
                    }
                }
            break;

            default:
            break;
        }

        add_size_property(properties, ctx, id);
    }

    StorageTexture image;
    TextureView view;
    const AssetId id;

    core::Vector<std::pair<core::String, core::String>> properties;
};

struct ThumbmailSceneData : NonMovable, public ContextLinked {
    ThumbmailSceneData(ContextPtr ctx) : ContextLinked(ctx), view(&world) {
        const float intensity = 1.0f;

        {
            const ecs::EntityId light_id = world.create_entity(DirectionalLightArchetype());
            DirectionalLightComponent* light_comp = world.component<DirectionalLightComponent>(light_id);
            light_comp->direction() = math::Vec3{0.0f, 0.3f, -1.0f};
            light_comp->intensity() = 3.0f * intensity;
        }
        {
            const ecs::EntityId light_id = world.create_entity(PointLightArchetype());
            world.component<TransformableComponent>(light_id)->position() = math::Vec3(0.75f, -0.5f, 0.5f);
            PointLightComponent* light = world.component<PointLightComponent>(light_id);
            light->color() = k_to_rbg(2500.0f);
            light->intensity() = 1.5f * intensity;
            light->falloff() = 0.5f;
            light->radius() = 2.0f;
        }
        {
            const ecs::EntityId light_id = world.create_entity(PointLightArchetype());
            world.component<TransformableComponent>(light_id)->position() = math::Vec3(-0.75f, -0.5f, 0.5f);
            PointLightComponent* light = world.component<PointLightComponent>(light_id);
            light->color() = k_to_rbg(10000.0f);
            light->intensity() = 1.5f * intensity;
            light->falloff() = 0.5f;
            light->radius() = 2.0f;
        }

        {
            const ecs::EntityId sky_id = world.create_entity(ecs::StaticArchetype<SkyLightComponent>());
            world.component<SkyLightComponent>(sky_id)->probe() = device_resources(device()).ibl_probe();
        }

        /*if(background) {
            ecs::EntityId bg_id = world.create_entity(StaticMeshArchetype());
            StaticMeshComponent* mesh_comp = world.component<StaticMeshComponent>(bg_id);
            *mesh_comp = StaticMeshComponent(dptr->device_resources()[DeviceResources::SweepMesh], dptr->device_resources()[DeviceResources::EmptyMaterial]);
        }*/

        view.camera().set_view(math::look_at(math::Vec3(0.0f, -1.0f, 1.0f), math::Vec3(0.0f), math::Vec3(0.0f, 0.0f, 1.0f)));
    }

    void add_mesh(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat) {
        ecs::EntityId entity = world.create_entity(StaticMeshArchetype());
        StaticMeshComponent* mesh_comp = world.component<StaticMeshComponent>(entity);
        *mesh_comp = StaticMeshComponent(mesh, mat);

        TransformableComponent* trans_comp = world.component<TransformableComponent>(entity);
        trans_comp->transform() = center_to_camera(mesh->aabb());
    }

    void add_prefab(const ecs::EntityPrefab& prefab)  {
        ecs::EntityId entity = world.create_entity(prefab);

        if(StaticMeshComponent* mesh_comp = world.component<StaticMeshComponent>(entity)) {
            if(TransformableComponent* trans_comp = world.component<TransformableComponent>(entity)) {
                trans_comp->transform() = center_to_camera(mesh_comp->compute_aabb());
            }
        }
    }

    ecs::EntityWorld world;
    SceneView view;
};




ThumbmailCache::ThumbmailCache(ContextPtr ctx, usize size) :
        ContextLinked(ctx),
        _size(size),
        _resource_pool(std::make_shared<FrameGraphResourcePool>(ctx->device())) {
}

ThumbmailCache::~ThumbmailCache() {

}

void ThumbmailCache::clear() {
    const auto lock = y_profile_unique_lock(_lock);
    _thumbmails.clear();
}

math::Vec2ui ThumbmailCache::thumbmail_size() const {
    return math::Vec2ui(_size);
}

ThumbmailCache::Thumbmail ThumbmailCache::get_thumbmail(AssetId asset) {
    y_profile();
    if(asset == AssetId::invalid_id()) {
        return Thumbmail{};
    }

    Y_TODO(Handle reloading)

    {
        const auto lock = y_profile_unique_lock(_lock);
        if(auto it = _thumbmails.find(asset); it != _thumbmails.end()) {
            if(it->second) {
                return Thumbmail{&it->second->view, it->second->properties};
            } else {
                return Thumbmail{};
            }
        }
        _thumbmails[asset] = nullptr;
    }

    request_thumbmail(asset);
    return Thumbmail{};
}

void ThumbmailCache::request_thumbmail(AssetId id) {
    y_profile();

    y_debug_assert([&] {
        const std::unique_lock lock(_lock);
        const auto it = _thumbmails.find(id);
        return it != _thumbmails.end() && it->second == nullptr;
    }());

    const AssetType asset_type = context()->asset_store().asset_type(id).unwrap_or(AssetType::Unknown);
    switch(asset_type) {
        case AssetType::Mesh:
            _render_thread.schedule([id, this] {
                if(const auto& mesh = context()->loader().load<StaticMesh>(id); !mesh.is_failed()) {
                    CmdBufferRecorder rec = create_disposable_cmd_buffer(device());
                    ThumbmailSceneData scene(context());
                    scene.add_mesh(mesh, device_resources(device())[DeviceResources::EmptyMaterial]);
                    submit_and_set(rec, render_thumbmail(rec, id, scene));
                } else {
                    log_msg(fmt("Failed to load asset with id: %", id), Log::Error);
                }
            });
        break;

        case AssetType::Material:
            _render_thread.schedule([id, this] {
                if(const auto& material = context()->loader().load<Material>(id); !material.is_failed()) {
                    CmdBufferRecorder rec = create_disposable_cmd_buffer(device());
                    ThumbmailSceneData scene(context());
                    scene.add_mesh(device_resources(device())[DeviceResources::SphereMesh], material);
                    submit_and_set(rec, render_thumbmail(rec, id, scene));
                } else {
                    log_msg(fmt("Failed to load asset with id: %", id), Log::Error);
                }
            });
        break;

        case AssetType::Image:
            _render_thread.schedule([id, this] {
                if(const auto& texture = context()->loader().load<Texture>(id); !texture.is_failed()) {
                    CmdBufferRecorder rec = create_disposable_cmd_buffer(device());
                    submit_and_set(rec, render_thumbmail(rec, texture));
                } else {
                    log_msg(fmt("Failed to load asset with id: %", id), Log::Error);
                }
            });
        break;

        case AssetType::Prefab:
            _render_thread.schedule([id, this] {
                if(const auto& prefab = context()->loader().load<ecs::EntityPrefab>(id); !prefab.is_failed()) {
                    CmdBufferRecorder rec = create_disposable_cmd_buffer(device());
                    ThumbmailSceneData scene(context());
                    scene.add_prefab(*prefab);
                    submit_and_set(rec, render_thumbmail(rec, id, scene));
                } else {
                    log_msg(fmt("Failed to load asset with id: %", id), Log::Error);
                }
            });
        break;

        default:
            log_msg(fmt("Unknown asset type % for %.", asset_type, id.id()), Log::Error);
        break;
    }
}

void ThumbmailCache::submit_and_set(CmdBufferRecorder& recorder, std::unique_ptr<ThumbmailData> thumb) {
    y_profile();
    std::move(recorder).submit<SyncPolicy::Sync>();

    const auto lock = y_profile_unique_lock(_lock);
    auto& thumbmail = _thumbmails[thumb->id];
    y_debug_assert(thumbmail == nullptr);
    thumbmail = std::move(thumb);
}

std::unique_ptr<ThumbmailData> ThumbmailCache::render_thumbmail(CmdBufferRecorder& recorder, const AssetPtr<Texture>& tex) const {
    auto thumbmail = std::make_unique<ThumbmailData>(context(), _size, tex.id());

    {
        const DescriptorSet set(device(), {Descriptor(*tex, SamplerType::LinearClamp), Descriptor(StorageView(thumbmail->image))});
        recorder.dispatch_size(device_resources(device())[DeviceResources::CopyProgram],  math::Vec2ui(_size), {set});
    }

    return thumbmail;
}

std::unique_ptr<ThumbmailData> ThumbmailCache::render_thumbmail(CmdBufferRecorder& recorder, AssetId id, const ThumbmailSceneData& scene) const {
    auto thumbmail = std::make_unique<ThumbmailData>(context(), _size, id);

    {
        const auto region = recorder.region("Thumbmail cache render");

        FrameGraph graph(_resource_pool);
        RendererSettings settings;
        settings.tone_mapping.auto_exposure = false;
        const DefaultRenderer renderer = DefaultRenderer::create(graph, scene.view, thumbmail->image.size(), settings);

        const FrameGraphImageId output_image = renderer.tone_mapping.tone_mapped;
        {
            FrameGraphPassBuilder builder = graph.add_pass("Thumbmail copy pass");
            builder.add_uniform_input(output_image);
            builder.add_uniform_input(renderer.gbuffer.depth);
            builder.add_external_input(StorageView(thumbmail->image));
            builder.set_render_func([=](CmdBufferRecorder& rec, const FrameGraphPass* self) {
                    rec.dispatch_size(context()->resources()[EditorResources::DepthAlphaProgram], math::Vec2ui(_size), {self->descriptor_sets()[0]});
                });
        }

        std::move(graph).render(recorder);
    }


    return thumbmail;
}



}

