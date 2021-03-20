/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "Preview.h"
#include "AssetSelector.h"

#include <editor/EditorApplication.h>
#include <editor/EditorWorld.h>
#include <editor/Settings.h>
#include <editor/utils/ui.h>

#include <yave/renderer/DefaultRenderer.h>
#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/images/ImageData.h>
#include <yave/meshes/MeshData.h>
#include <yave/material/Material.h>
#include <yave/graphics/images/IBLProbe.h>
#include <yave/framegraph/FrameGraphResourcePool.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/color.h>


#include <external/imgui/yave_imgui.h>

namespace editor {


Preview::Preview() :
        Widget(ICON_FA_BRUSH " Material Preview", ImGuiWindowFlags_NoScrollbar),
        _resource_pool(std::make_shared<FrameGraphResourcePool>(app_device())) {

    set_object(PreviewObject::Sphere);
}

Preview::~Preview() {

}

void Preview::refresh() {
    _world->flush_reload();
}

void Preview::set_material(const AssetPtr<Material>& material) {
    if(_material == material) {
        return;
    }

    _material = material;
    reset_world();
}

void Preview::set_object(const AssetPtr<StaticMesh>& mesh) {
    _mesh = mesh;
    reset_world();
}

void Preview::set_object(PreviewObject obj) {
    switch(obj) {
        case PreviewObject::Cube:
            _mesh = device_resources(app_device())[DeviceResources::CubeMesh];
        break;

        default:
            _mesh = device_resources(app_device())[DeviceResources::SphereMesh];
    }
    reset_world();
}

const AssetPtr<Material>& Preview::material() const {
    return _material;
}

void Preview::update_camera() {
    if(ImGui::IsMouseDown(0) && /*is_mouse_inside()*/ ImGui::IsWindowHovered()) {
        math::Vec2 delta = math::Vec2(ImGui::GetIO().MouseDelta) / math::Vec2(content_size());
        delta *= app_settings().camera.trackball_sensitivity;

        const float pi_2 = (math::pi<float> * 0.5f) - 0.001f;
        _angle.y() = std::clamp(_angle.y() + delta.y(), -pi_2, pi_2);
        _angle.x() += delta.x();
    }

    {
        const float cos_y = std::cos(_angle.y());
        const math::Vec3 cam = math::Vec3(std::sin(_angle.x()) * cos_y, std::cos(_angle.x()) * cos_y, std::sin(_angle.y()));

        _view.camera().set_view(math::look_at(cam * _cam_distance, math::Vec3(), math::Vec3(0.0f, 0.0f, 1.0f)));
    }
}


void Preview::reset_world() {
    _world = std::make_unique<EditorWorld>(asset_loader());
    _view = SceneView(_world.get());

    {
        const ecs::EntityId sky_id = _world->create_entity<SkyLightComponent>();
        _world->component<SkyLightComponent>(sky_id)->probe() = _ibl_probe
            ? _ibl_probe
            : device_resources(app_device()).ibl_probe();
    }

    if(!_mesh.is_empty() && !_material.is_empty()) {
        const ecs::EntityId id = _world->create_entity<StaticMeshComponent>();
        *_world->component<StaticMeshComponent>(id) = StaticMeshComponent(_mesh, _material);

        const float radius = _mesh->radius();
        _cam_distance = std::sqrt(3.0f * radius * radius) * 1.5f;
    }
}

void Preview::draw_mesh_menu() {
    if(ImGui::BeginPopup("##contextmenu")) {
        if(ImGui::MenuItem("Sphere")) {
            set_object(PreviewObject::Sphere);
        }
        if(ImGui::MenuItem("Cube")) {
            set_object(PreviewObject::Cube);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Custom")) {
            add_child_widget<AssetSelector>(AssetType::Mesh)->set_selected_callback(
                [this](AssetId id) {
                    if(auto mesh = asset_loader().load_res<StaticMesh>(id)) {
                        set_object(mesh.unwrap());
                    }
                    return true;
                });
        }
        ImGui::EndPopup();
    }
}

void Preview::on_gui() {
    y_profile();

    update_camera();

    TextureView* output = nullptr;

    {
        FrameGraph graph(_resource_pool);
        const DefaultRenderer renderer = DefaultRenderer::create(graph, _view, content_size());

        FrameGraphPassBuilder builder = graph.add_pass("ImGui texture pass");

        const auto output_image = builder.declare_copy(renderer.lighting.lit);
        Y_TODO(This is not enough to properly sync)
        builder.add_image_input_usage(output_image, ImageUsage::TextureBit);
        builder.set_render_func([=, &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
                auto out = std::make_unique<TextureView>(self->resources().image<ImageUsage::TextureBit>(output_image));
                output = out.get();
                recorder.keep_alive(std::move(out));
            });


        CmdBufferRecorder& recorder = application()->recorder();
        const auto region = recorder.region("Peview render", math::Vec4(0.7f, 0.7f, 0.7f, 1.0f));
        std::move(graph).render(recorder);
    }

    if(output) {
        const math::Vec2 top_left = ImGui::GetCursorPos();
        const float width = ImGui::GetContentRegionAvail().x;
        ImGui::Image(output, ImVec2(width, width));
        const math::Vec2 bottom = ImGui::GetCursorPos();

        ImGui::SetCursorPos(top_left + math::Vec2(4.0f));
        if(ImGui::Button(ICON_FA_CIRCLE)) {
            add_child_widget<AssetSelector>(AssetType::Image)->set_selected_callback(
                [this](AssetId id) {
                    if(const auto tex = asset_loader().load_res<IBLProbe>(id)) {
                        _ibl_probe = tex.unwrap();
                        reset_world();
                    }
                    return true;
                });
        }

        ImGui::SameLine();

        if(ImGui::Button(ICON_FA_CUBE)) {
            ImGui::OpenPopup("##contextmenu");
        }

        draw_mesh_menu();

        ImGui::SetCursorPos(bottom);
    }
}

}
