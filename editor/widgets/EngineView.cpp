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

#include "EngineView.h"

#include <editor/Picker.h>
#include <editor/Settings.h>
#include <editor/Selection.h>
#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>
#include <editor/EditorApplication.h>
#include <editor/utils/CameraController.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphResourcePool.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/utils/color.h>

#include <external/imgui/yave_imgui.h>


namespace editor {

static auto snapping_distances() {
    return std::array {
        0.125f,
        0.25f,
        0.5f,
        1.0f,
        2.5f,
        5.0f,
        10.0f,
    };
}

static auto snapping_angles() {
    return std::array {
        1.0f,
        5.0f,
        15.0f,
        30.0f,
        45.0f,
        60.0f,
        90.0f
    };
}

static bool is_clicked() {
    return ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2);
}

static auto standard_resolutions() {
    static std::array<std::pair<const char*, math::Vec2ui>, 3> resolutions = {{
        {"1080p",   {1920, 1080}},
        {"1440p",   {2560, 1440}},
        {"4k",      {3840, 2160}}
    }};

    return resolutions;
}





EngineView::EngineView() :
        Widget(ICON_FA_DESKTOP " Engine View", ImGuiWindowFlags_MenuBar),
        _resource_pool(std::make_shared<FrameGraphResourcePool>()),
        _scene_view(&current_world()),
        _camera_controller(std::make_unique<HoudiniCameraController>()),
        _gizmo(&_scene_view) {
}

EngineView::~EngineView() {
    application()->unset_scene_view(&_scene_view);
}

bool EngineView::is_mouse_inside() const {
    const math::Vec2 mouse_pos = math::Vec2(ImGui::GetIO().MousePos) - math::Vec2(ImGui::GetWindowPos());
    const auto less = [](const math::Vec2& a, const math::Vec2& b) { return a.x() < b.x() && a.y() < b.y(); };
    return less(mouse_pos, ImGui::GetWindowContentRegionMax()) && less(ImGui::GetWindowContentRegionMin(), mouse_pos);
}

bool EngineView::is_focussed() const {
    return ImGui::IsWindowFocused();
}


// ---------------------------------------------- DRAW ----------------------------------------------

bool EngineView::before_gui() {
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, math::Vec4(0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, math::Vec4(0.0f));

    return true;
}

void EngineView::after_gui() {
    ImGui::PopStyleColor(2);
}

void EngineView::draw(CmdBufferRecorder& recorder) {
    TextureView* output = nullptr;
    FrameGraph graph(_resource_pool);

    const math::Vec2ui output_size = _resolution < 0 ? content_size() : standard_resolutions()[_resolution].second;

    const EditorRenderer renderer = EditorRenderer::create(graph, _scene_view, output_size, _settings);

    {
        const Texture& white = *device_resources()[DeviceResources::WhiteTexture];

        FrameGraphPassBuilder builder = graph.add_pass("ImGui texture pass");

        const auto output_image = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, output_size);

        const auto gbuffer = renderer.renderer.gbuffer;
        builder.add_image_input_usage(output_image, ImageUsage::TextureBit);
        builder.add_color_output(output_image);
        builder.add_inline_input(u32(_view), 0);
        builder.add_uniform_input(renderer.final, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.color, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input_with_default(renderer.renderer.ssao.ao, Descriptor(white), 0, PipelineStage::FragmentBit);
        builder.set_render_func([=, &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
                auto out = std::make_unique<TextureView>(self->resources().image<ImageUsage::TextureBit>(output_image));
                output = out.get();
                recorder.keep_alive(std::move(out));

                auto render_pass = recorder.bind_framebuffer(self->framebuffer());
                const MaterialTemplate* material = resources()[EditorResources::EngineViewMaterialTemplate];
                render_pass.bind_material(material, {self->descriptor_sets()[0]});

                render_pass.draw_array(3);
            });
    }

    if(!_disable_render) {
        std::move(graph).render(recorder);
    }

    if(output) {
        const math::Vec2 position = ImGui::GetWindowPos();
        ImGui::GetWindowDrawList()->AddImage(output,
            position + math::Vec2(ImGui::GetWindowContentRegionMin()),
            position + math::Vec2(ImGui::GetWindowContentRegionMax()));
    }

}

void EngineView::on_gui() {
    y_profile();

    update_proj();

    draw_menu_bar();
    draw(application()->recorder());
    _gizmo.draw();

    update();
}

void EngineView::draw_settings_menu() {
     if(ImGui::BeginMenu("Tone mapping")) {
        ToneMappingSettings& settings = _settings.renderer_settings.tone_mapping;
        ImGui::MenuItem("Auto exposure", nullptr, &settings.auto_exposure);

        // https://docs.unrealengine.com/en-US/Engine/Rendering/PostProcessEffects/AutomaticExposure/index.html
        float ev = exposure_to_EV100(settings.exposure);
        if(ImGui::SliderFloat("EV100", &ev, -10.0f, 10.0f)) {
            settings.exposure = EV100_to_exposure(ev);
        }

        if(ImGui::IsItemActive() || ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Exposure scale = %.3f", settings.exposure);
        }

        const char* tone_mappers[] = {"ACES", "Uncharted 2", "Reinhard", "None"};
        if(ImGui::BeginCombo("Tone mapper", tone_mappers[usize(settings.tone_mapper)])) {
            for(usize i = 0; i != sizeof(tone_mappers) / sizeof(tone_mappers[0]); ++i) {
                const bool selected = usize(settings.tone_mapper) == i;
                if(ImGui::Selectable(tone_mappers[i], selected)) {
                    settings.tone_mapper = ToneMappingSettings::ToneMapper(i);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("SSAO")) {
        SSAOSettings& settings = _settings.renderer_settings.ssao;

        bool enabled = settings.method != SSAOSettings::SSAOMethod::None;
        ImGui::Checkbox("Enabled", &enabled);

        int levels = int(settings.level_count);

        ImGui::SliderInt("Levels", &levels, 2, 8);
        ImGui::SliderFloat("Blur tolerance", &settings.blur_tolerance, 1.0f, 8.0f);
        ImGui::SliderFloat("Upsample tolerance", &settings.upsample_tolerance, 1.0f, 12.0f);
        ImGui::SliderFloat("Noise filter", &settings.noise_filter_tolerance, 0.0f, 8.0f);

        settings.level_count = levels;
        settings.method = enabled ? SSAOSettings::SSAOMethod::MiniEngine : SSAOSettings::SSAOMethod::None;

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Bloom")) {
        BloomSettings& settings = _settings.renderer_settings.bloom;
        int pyramids = int(settings.pyramids);
        bool scatter = settings.type == BloomSettings::Scattering;

        ImGui::Checkbox("Use scattering", &scatter);
        ImGui::SliderFloat("Scattering intensity", &settings.scatter_intensity, 0.01f, 1.0f, "%.3f");
        ImGui::Separator();
        ImGui::SliderFloat("Power", &settings.power, 0.01f, 10.0f, "%.3f");
        ImGui::SliderFloat("Threshold", &settings.threshold, 0.0f, 1.0f);
        ImGui::SliderInt("Pyramids", &pyramids, 1, 8);
        ImGui::Separator();
        ImGui::SliderFloat("Blur sigma", &settings.blur.sigma, 0.01f, 6.0f);

        settings.pyramids = usize(pyramids);
        settings.type = scatter ? BloomSettings::Scattering : BloomSettings::Additive;

        ImGui::EndMenu();
    }


    if(ImGui::BeginMenu("Lighting")) {
        LightingSettings& settings = _settings.renderer_settings.lighting;

        ImGui::Checkbox("Use compute", &settings.use_compute_for_locals);

        ImGui::EndMenu();
    }
}

void EngineView::draw_menu_bar() {
    if(ImGui::BeginMenuBar()) {
        if(ImGui::BeginMenu("Render")) {
            ImGui::MenuItem("Editor entities", nullptr, &_settings.show_editor_entities);

            ImGui::Separator();
            {
                const char* output_names[] = {
                        "Lit", "Albedo", "Normals", "Metallic", "Roughness", "Depth", "AO"
                    };
                for(usize i = 0; i != usize(RenderView::MaxRenderViews); ++i) {
                    bool selected = usize(_view) == i;
                    ImGui::MenuItem(output_names[i], nullptr, &selected);
                    if(selected) {
                        _view = RenderView(i);
                    }
                }
            }

            ImGui::Separator();
            ImGui::MenuItem("Disable render", nullptr, &_disable_render);

            ImGui::Separator();
            if(ImGui::BeginMenu("Rendering Settings")) {
                draw_settings_menu();
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Camera")) {
            if(ImGui::MenuItem("Reset camera")) {
                _scene_view.camera().set_view(Camera().view_matrix());
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Resolution")) {
            if(ImGui::MenuItem("Viewport", "", _resolution < 0)) {
                _resolution = -1;
            }
            ImGui::Separator();

            const auto resolutions = standard_resolutions();
            for(isize i = 0; i != isize(resolutions.size()); ++i) {
                if(ImGui::MenuItem(resolutions[i].first, "", _resolution == i)) {
                    _resolution = i;
                }
            }

            ImGui::EndMenu();
        }

        draw_gizmo_tool_bar();

        if(_resolution >= 0) {
            ImGui::Separator();
            ImGui::TextUnformatted(standard_resolutions()[_resolution].first);
        }

        ImGui::EndMenuBar();
    }
}

void EngineView::draw_gizmo_tool_bar() {
    ImGui::Separator();

    auto gizmo_mode = _gizmo.mode();
    auto gizmo_space = _gizmo.space();
    auto snapping = _gizmo.snapping();
    auto rot_snap = _gizmo.rotation_snapping();

    {
        if(ImGui::MenuItem(ICON_FA_ARROWS_ALT, nullptr, false, gizmo_mode != Gizmo::Translate)) {
            gizmo_mode = Gizmo::Translate;
        }
        if(ImGui::MenuItem(ICON_FA_SYNC_ALT, nullptr, false, gizmo_mode != Gizmo::Rotate)) {
            gizmo_mode = Gizmo::Rotate;
        }
    }
    {
        if(ImGui::MenuItem(ICON_FA_MOUNTAIN, nullptr, false, gizmo_space != Gizmo::World)) {
            gizmo_space = Gizmo::World;
        }
        if(ImGui::MenuItem(ICON_FA_CUBE, nullptr, false, gizmo_space != Gizmo::Local)) {
            gizmo_space = Gizmo::Local;
        }
    }
    {
        ImGui::Separator();
        if(ImGui::BeginMenu(ICON_FA_MAGNET)) {
            std::array<char, 16> buffer = {};

            for(const float d : snapping_distances()) {
                std::snprintf(buffer.data(), buffer.size(), "%g m", d);
                const bool selected = d == snapping;
                if(ImGui::MenuItem(buffer.data(), nullptr, selected)) {
                    snapping = selected ? 0.0f : d;
                }
            }

            ImGui::Separator();

            for(const float d : snapping_angles()) {
                std::snprintf(buffer.data(), buffer.size(), "%g°", d);
                const float angle = math::to_rad(d);
                const bool selected = angle == rot_snap;
                if(ImGui::MenuItem(buffer.data(), nullptr, selected)) {
                    rot_snap = selected ? 0.0f : angle;
                }
            }

            ImGui::EndMenu();
        }
    }



    if(is_focussed()) {
        const UiSettings& settings = app_settings().ui;
        if(ImGui::IsKeyReleased(int(settings.change_gizmo_mode))) {
            gizmo_mode = Gizmo::Mode(!usize(gizmo_mode));
        }
        if(ImGui::IsKeyReleased(int(settings.change_gizmo_space))) {
            gizmo_space = Gizmo::Space(!usize(gizmo_space));
        }
    }

    _gizmo.set_mode(gizmo_mode);
    _gizmo.set_space(gizmo_space);
    _gizmo.set_snapping(snapping);
    _gizmo.set_rotation_snapping(rot_snap);
}




// ---------------------------------------------- UPDATE ----------------------------------------------

void EngineView::update() {
    _gizmo.set_allow_drag(true);

    const bool hovered = ImGui::IsWindowHovered() && is_mouse_inside();

    bool focussed = ImGui::IsWindowFocused();
    if(hovered && is_clicked()) {
        ImGui::SetWindowFocus();
        update_picking();
        focussed = true;
    }

    if(focussed) {
        application()->set_scene_view(&_scene_view);
    }

    if(hovered && !_gizmo.is_dragging() && _camera_controller) {
        auto& camera = _scene_view.camera();
        _camera_controller->process_generic_shortcuts(camera);
        if(focussed) {
            _camera_controller->update_camera(camera, content_size());
        }
    }
}

void EngineView::update_proj() {
    const CameraSettings& settings = app_settings().camera;
    math::Vec2ui viewport_size = content_size();

    const float fov = math::to_rad(settings.fov);
    const auto proj = math::perspective(fov, float(viewport_size.x()) / float(viewport_size.y()), settings.z_near);
    _scene_view.camera().set_proj(proj);
}


void EngineView::update_picking() {
    const math::Vec2ui viewport_size = content_size();
    const math::Vec2 offset = ImGui::GetWindowPos();
    const math::Vec2 mouse = ImGui::GetIO().MousePos;
    const math::Vec2 uv = (mouse - offset - math::Vec2(ImGui::GetWindowContentRegionMin())) / math::Vec2(viewport_size);

    if(uv.x() < 0.0f || uv.y() < 0.0f ||
       uv.x() > 1.0f || uv.y() > 1.0f) {

        return;
    }

    const PickingResult picking_data = Picker::pick_sync(_scene_view, uv, viewport_size);
    if(_camera_controller && _camera_controller->viewport_clicked(picking_data)) {
        // event has been eaten by the camera controller, don't proceed further
        _gizmo.set_allow_drag(false);
        return;
    }

    if(ImGui::IsMouseClicked(0)) {
        if(!_gizmo.is_dragging()) {
            ecs::EntityId picked_id = picking_data.hit() ? current_world().id_from_index(picking_data.entity_index) : ecs::EntityId();
            selection().set_selected(picked_id);
        }
    }
}

}

