/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>
#include <editor/EditorApplication.h>
#include <editor/utils/CameraController.h>
#include <editor/utils/ui.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphResourcePool.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdTimingRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/utils/color.h>

#include <editor/utils/ui.h>

namespace editor {

static bool is_clicked() {
    return ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
}

static auto standard_resolutions() {
    static std::array<std::pair<const char*, math::Vec2ui>, 3> resolutions = {{
        {"1080p",   {1920, 1080}},
        {"1440p",   {2560, 1440}},
        {"4k",      {3840, 2160}}
    }};

    return resolutions;
}

static bool keep_taa(EngineView::RenderView view) {
    switch(view) {
        case EngineView::RenderView::Lit:
        case EngineView::RenderView::Motion:
            return true;
        break;

        default:
        break;
    }

    return false;
}



EngineView::EngineView() :
        Widget(ICON_FA_DESKTOP " Engine View"),
        _resource_pool(std::make_shared<FrameGraphResourcePool>()),
        _camera_controller(std::make_unique<HoudiniCameraController>()),
        _tr_gizmo(&_scene_view),
        _rot_gizmo(&_scene_view),
        _orientation_gizmo(&_scene_view) {
}

EngineView::~EngineView() {
    application()->unset_scene_view(&_scene_view);
}

CmdTimingRecorder* EngineView::timing_recorder() const {
    if(!_time_recs.is_empty() && _time_recs.first()->is_data_ready()) {
        return _time_recs.first().get();
    }
    return nullptr;
}

bool EngineView::is_mouse_inside() const {
    const math::Vec2 mouse_pos = math::Vec2(ImGui::GetIO().MousePos) - math::Vec2(ImGui::GetWindowPos());
    const auto less = [](const math::Vec2& a, const math::Vec2& b) { return a.x() < b.x() && a.y() < b.y(); };
    return less(mouse_pos, ImGui::GetWindowContentRegionMax()) && less(ImGui::GetWindowContentRegionMin(), mouse_pos);
}

bool EngineView::is_focussed() const {
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
}

bool EngineView::should_keep_alive() const {
    for(const auto& t : _time_recs) {
        if(!t->is_data_ready()) {
            return true;
        }
    }
    return false;
}

bool EngineView::is_dragging_gizmo() const {
    return _orientation_gizmo.is_dragging() || _tr_gizmo.is_dragging() || _rot_gizmo.is_dragging();
}

void EngineView::set_allow_dragging_gizmo(bool allow) {
    _orientation_gizmo.set_allow_dragging(allow);
    _tr_gizmo.set_allow_dragging(allow);
    _rot_gizmo.set_allow_dragging(allow);
}


// ---------------------------------------------- DRAW ----------------------------------------------

bool EngineView::before_gui() {
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_HeaderActive));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, math::Vec2());

    return Widget::before_gui();
}

void EngineView::after_gui() {
    Widget::after_gui();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

void EngineView::draw(CmdBufferRecorder& recorder) {
    while(_time_recs.size() >= 2) {
        if(_time_recs[0]->is_data_ready() && _time_recs[1]->is_data_ready()) {
            _time_recs.pop_front();
        } else {
            break;
        }
    }

    const math::Vec2ui output_size = _resolution < 0 ? content_size() : standard_resolutions()[_resolution].second;

    UiTexture output;
    FrameGraph graph(_resource_pool);


    EditorRendererSettings settings = _settings;
    {
        settings.show_debug_drawer = app_settings().debug.display_debug_drawer;
        settings.renderer_settings.taa.enable &= keep_taa(_view);
    }


    const EditorRenderer renderer = EditorRenderer::create(graph, _scene_view, output_size, settings);
    {
        const Texture& white = *device_resources()[DeviceResources::WhiteTexture];

        FrameGraphComputePassBuilder builder = graph.add_compute_pass("ImGui texture pass");

        const auto output_image = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, output_size);

        const auto gbuffer = renderer.renderer.gbuffer;
        builder.add_input_usage(output_image, ImageUsage::TransferSrcBit);
        builder.add_color_output(output_image);
        builder.add_inline_input(InlineDescriptor(_view));
        builder.add_uniform_input(renderer.final);
        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.motion);
        builder.add_uniform_input(gbuffer.color);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_uniform_input_with_default(renderer.renderer.ssao.ao, Descriptor(white));
        builder.set_render_func([=, &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            {
                auto render_pass = recorder.bind_framebuffer(self->framebuffer());
                const MaterialTemplate* material = resources()[EditorResources::EngineViewMaterialTemplate];
                render_pass.bind_material_template(material, self->descriptor_sets());
                render_pass.draw_array(3);
            }
            const auto& src = self->resources().image_base(output_image);
            output = UiTexture(src.format(), src.image_size().to<2>());
            recorder.copy(src, output.texture());
        });
    }

    {
        CmdTimingRecorder* time_rec = _time_recs.emplace_back(std::make_unique<CmdTimingRecorder>(recorder)).get();
        graph.render(recorder, time_rec);
    }

    if(output) {
        ImGui::Image(output.to_imgui(), content_size());
    }
}



// ---------------------------------------------- UPDATE ----------------------------------------------

void EngineView::update() {
    const bool hovered = ImGui::IsWindowHovered() && is_mouse_inside();

    set_allow_dragging_gizmo(hovered);

    if(!hovered) {
        return;
    }

    bool focussed = ImGui::IsWindowFocused();
    if(is_clicked()) {
        ImGui::SetWindowFocus();
        update_picking();
        focussed = true;
    }

    if(focussed) {
        application()->set_scene_view(&_scene_view);
    }

    if(!is_dragging_gizmo() && _camera_controller) {
        auto& camera = _scene_view.camera();
        _camera_controller->process_generic_shortcuts(camera);
        if(focussed) {
            _camera_controller->update_camera(camera, content_size());
        }
    }
}

void EngineView::update_scene_view() {
    if(!_scene_view.has_world() || &_scene_view.world() != &current_world()) {
        _scene_view = SceneView(&current_world());
    }

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
        set_allow_dragging_gizmo(false);
        return;
    }

    if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if(!is_dragging_gizmo()) {
            const ecs::EntityId picked_id = picking_data.hit() ? current_world().id_from_index(picking_data.entity_index) : ecs::EntityId();
            current_world().toggle_selected(picked_id, !ImGui::GetIO().KeyCtrl);
        }
    }
}

void EngineView::make_drop_target() {
    if(ImGui::BeginDragDropTarget()) {
        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(imgui::drag_drop_path_id)) {
            const std::string_view name = static_cast<const char*>(payload->Data);
            current_world().set_selected(current_world().add_prefab(name));
        }
        ImGui::EndDragDropTarget();
    }
}



// ---------------------------------------------- GUI ----------------------------------------------

void EngineView::on_gui() {
    if(ImGui::BeginChild("##view")) {
        update_scene_view();

        const math::Vec2 cursor = ImGui::GetCursorPos();

        draw(application()->recorder());
        make_drop_target();

        {
            ImGui::SetCursorPos(cursor + math::Vec2(ImGui::GetStyle().IndentSpacing * 0.5f));
            draw_toolbar_and_gizmos();
        }

        if(ImGui::BeginPopup("##menu")) {
            draw_menu();
            ImGui::EndPopup();
        }

        if(ImGui::BeginPopup("##resolutions")) {
            draw_resolution_menu();
            ImGui::EndPopup();
        }

        update();
    }
    ImGui::EndChild();
}


void EngineView::draw_toolbar_and_gizmos() {
    struct GizmoButton  {
        const char* icon = nullptr;
        GizmoBase* gizmo = nullptr;
    };

    const std::array<GizmoButton, usize(GizmoType::Max)> gizmo_buttons = {
        GizmoButton { ICON_FA_ARROWS_ALT, &_tr_gizmo },
        GizmoButton { ICON_FA_SYNC_ALT, &_rot_gizmo }
    };


    if(!is_dragging_gizmo()) {
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFFFFFF);
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_PopupBg));

        if(ImGui::Button(ICON_FA_ADJUST)) {
            ImGui::OpenPopup("##menu");
        }

        ImGui::SameLine();

        for(usize i = 0; i != gizmo_buttons.size(); ++i) {
            const bool is_selected = GizmoType(i) == _gizmo;
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(is_selected ? ImGuiCol_CheckMark : ImGuiCol_Text));

            const GizmoButton& button = gizmo_buttons[i];
            if(ImGui::Button(button.icon)) {
                _gizmo = GizmoType(i);
            }

            ImGui::PopStyleColor();
            ImGui::SameLine();
        }

        if(_resolution >= 0) {
            if(ImGui::Button(standard_resolutions()[_resolution].first)) {
                ImGui::OpenPopup("##resolutions");
            }
        }

        ImGui::PopStyleColor(2);
    }

    if(is_focussed() && ImGui::IsKeyPressed(to_imgui_key(app_settings().ui.change_gizmo_mode))) {
        _gizmo = GizmoType((usize(_gizmo) + 1) % usize(GizmoType::Max));
    }

    gizmo_buttons[usize(_gizmo)].gizmo->draw();
    _orientation_gizmo.draw();
}


void EngineView::draw_menu() {
    {
        ImGui::MenuItem("Editor entities", nullptr, &_settings.show_editor_entities);

        ImGui::Separator();
        {
            const char* output_names[] = {"Lit", "Albedo", "Normals", "Metallic", "Roughness", "Depth", "Motion", "AO"};
            for(usize i = 0; i != usize(RenderView::Max); ++i) {
                bool selected = usize(_view) == i;
                ImGui::MenuItem(output_names[i], nullptr, &selected);
                if(selected) {
                    _view = RenderView(i);
                }
            }
        }
    }

    ImGui::Separator();

    if(ImGui::BeginMenu("Rendering Settings")) {
        draw_settings_menu();
        ImGui::EndMenu();
    }

    ImGui::Separator();

    if(ImGui::BeginMenu("Camera")) {
        if(ImGui::MenuItem("Reset camera")) {
            _scene_view.camera().set_view(Camera().view_matrix());
        }
        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Resolution")) {
        draw_resolution_menu();
        ImGui::EndMenu();
    }
}

void EngineView::draw_resolution_menu() {
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
}

void EngineView::draw_settings_menu() {
    if(ImGui::BeginMenu("Tone mapping")) {
        ToneMappingSettings& settings = _settings.renderer_settings.tone_mapping;
        ImGui::Checkbox("Auto exposure", &settings.auto_exposure);

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

        ImGui::Separator();

        ImGui::Checkbox("Show histogram", &settings.debug_exposure);

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("SSAO")) {
        SSAOSettings& settings = _settings.renderer_settings.ssao;

        const char* methods[] = {"MiniEngine", "None"};
        if(ImGui::BeginCombo("SSAO", methods[usize(settings.method)])) {
            for(usize i = 0; i != sizeof(methods) / sizeof(methods[0]); ++i) {
                const bool selected = usize(settings.method) == i;
                if(ImGui::Selectable(methods[i], selected)) {
                    settings.method = SSAOSettings::SSAOMethod(i);
                }
            }
            ImGui::EndCombo();
        }

        int levels = int(settings.level_count);
        ImGui::SliderInt("Levels", &levels, 2, 8);
        ImGui::SliderFloat("Blur tolerance", &settings.blur_tolerance, 1.0f, 8.0f);
        ImGui::SliderFloat("Upsample tolerance", &settings.upsample_tolerance, 1.0f, 12.0f);
        ImGui::SliderFloat("Noise filter", &settings.noise_filter_tolerance, 0.0f, 8.0f);

        settings.level_count = levels;

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Bloom")) {
        BloomSettings& settings = _settings.renderer_settings.bloom;

        int pyramids = int(settings.pyramids);
        ImGui::SliderInt("Pyramids", &pyramids, 1, 8);
        settings.pyramids = usize(pyramids);

        ImGui::Separator();

        ImGui::SliderFloat("Radius", &settings.radius, 0.001f, 0.01f, "%.3f");

        ImGui::SliderFloat("Intensity", &settings.intensity, 0.0f, 0.2f, "%.3f");

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("Lighting")) {
        LightingSettings& settings = _settings.renderer_settings.lighting;

        ImGui::Checkbox("Use compute", &settings.use_compute_for_locals);

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("TAA")) {
        TAASettings& settings = _settings.renderer_settings.taa;

        ImGui::Checkbox("Enable TAA", &settings.enable);

        ImGui::Separator();

        ImGui::Checkbox("Enable clamping", &settings.use_clamping);
        ImGui::Checkbox("Enable motion rejection", &settings.use_motion_rejection);

        ImGui::Separator();

        const char* weighting_names[] = {"None", "Luminance", "Log"};
        if(ImGui::BeginCombo("Weighting mode", weighting_names[usize(settings.weighting_mode)])) {
            for(usize i = 0; i != sizeof(weighting_names) / sizeof(weighting_names[0]); ++i) {
                const bool selected = usize(settings.weighting_mode) == i;
                if(ImGui::Selectable(weighting_names[i], selected)) {
                    settings.weighting_mode = TAASettings::WeightingMode(i);
                }
            }
            ImGui::EndCombo();
        }

        const char* jitter_names[] = {"Weyl", "R2"};
        if(ImGui::BeginCombo("Jitter", jitter_names[usize(settings.jitter)])) {
            for(usize i = 0; i != sizeof(jitter_names) / sizeof(jitter_names[0]); ++i) {
                const bool selected = usize(settings.jitter) == i;
                if(ImGui::Selectable(jitter_names[i], selected)) {
                    settings.jitter = TAASettings::JitterSeq(i);
                }
            }
            ImGui::EndCombo();
        }


        ImGui::Separator();

        ImGui::SliderFloat("Blending factor", &settings.blending_factor, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Jitter intensity", &settings.jitter_intensity, 0.0f, 2.0f, "%.2f");

        ImGui::EndMenu();
    }
}

}

