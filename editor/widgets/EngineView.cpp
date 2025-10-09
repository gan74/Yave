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

#include "EngineView.h"

#include <editor/Settings.h>
#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>
#include <editor/ImGuiPlatform.h>
#include <editor/utils/ui.h>

#include <yave/scene/EcsScene.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphResourcePool.h>
#include <yave/renderer/IdBufferPass.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdTimestampPool.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/utils/color.h>
#include <yave/utils/DirectDraw.h>

#include <editor/utils/ui.h>

namespace editor {



editor_action_enable("Reset camera",
    [] { last_focussed_widget_typed<EngineView>()->reset_camera(); },
    [] { return last_focussed_widget_typed<EngineView>() != nullptr; }
)






static bool is_clicked() {
    return ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
}

static auto standard_resolutions() {
    static const std::array<std::pair<const char*, math::Vec2ui>, 3> resolutions = {{
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
    unset_scene_view(&_scene_view);
}

void EngineView::reset_camera() {
   _scene_view.camera().set_view(Camera().view_matrix());
}

CmdTimestampPool* EngineView::timestamp_pool() const {
    if(!_timestamp_pools.is_empty() && _timestamp_pools.first()->is_ready()) {
        return _timestamp_pools.first().get();
    }
    return nullptr;
}

bool EngineView::is_mouse_inside() const {
    const auto less = [](const ImVec2& a, const ImVec2& b) { return a.x < b.x && a.y < b.y; };
    const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    return less(mouse_pos, ImGui::GetWindowPos() + ImGui::GetWindowSize()) && less(ImGui::GetWindowPos(), mouse_pos);
}

bool EngineView::is_focussed() const {
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
}

bool EngineView::should_keep_alive() const {
    for(const auto& t : _timestamp_pools) {
        if(!t->is_ready()) {
            return true;
        }
    }
    return false;
}

bool EngineView::is_dragging_gizmo() const {
    return _orientation_gizmo.is_dragging() || _tr_gizmo.is_dragging() || _rot_gizmo.is_dragging();
}

void EngineView::set_is_moving_camera(bool moving) {
    _moving_camera = moving;
    _orientation_gizmo.set_allow_dragging(!moving);
    _tr_gizmo.set_allow_dragging(!moving);
    _rot_gizmo.set_allow_dragging(!moving);
}


// ---------------------------------------------- DRAW ----------------------------------------------

bool EngineView::before_gui() {
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_HeaderActive));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{});

    return Widget::before_gui();
}

void EngineView::after_gui() {
    Widget::after_gui();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

void EngineView::draw(CmdBufferRecorder& recorder) {
    while(_timestamp_pools.size() >= 2) {
        if(_timestamp_pools[0]->is_ready() && _timestamp_pools[1]->is_ready()) {
            _timestamp_pools.pop_front();
        } else {
            break;
        }
    }

    const math::Vec2ui output_size = _resolution < 0 ? content_size() : standard_resolutions()[_resolution].second;

    DstTexture output;
    FrameGraph framegraph(_resource_pool);


    EditorRendererSettings settings = _settings;
    {
        settings.show_debug_drawer = app_settings().debug.display_debug_drawer;
        settings.renderer_settings.taa.enable &= keep_taa(_view);
    }

    const EditorRenderer renderer = EditorRenderer::create(framegraph, _scene_view, output_size, settings);
    {
        const Texture& white = *device_resources()[DeviceResources::WhiteTexture];

        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("ImGui texture pass");

        const auto output_image = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, output_size);

        const auto gbuffer = renderer.renderer.gbuffer;
        builder.add_input_usage(output_image, ImageUsage::TransferSrcBit);
        builder.add_color_output(output_image);
        builder.add_inline_input(_view);
        builder.add_uniform_input(renderer.final);
        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.motion);
        builder.add_uniform_input(gbuffer.color);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_uniform_input_with_default(renderer.renderer.ao.ao, Descriptor(white));
        builder.set_render_func([=, &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            {
                auto render_pass = recorder.bind_framebuffer(self->framebuffer());
                const MaterialTemplate* material = resources()[EditorResources::EngineViewMaterialTemplate];
                render_pass.bind_material_template(material, self->descriptor_set());
                render_pass.draw_array(3);
            }
            const auto& src = self->resources().image_base(output_image);
            output = DstTexture(src.format(), src.image_size().to<2>());
            recorder.copy(src, output);
        });
    }

    if(is_mouse_inside()) {
        const math::Vec2 mouse_pos = to_y(ImGui::GetIO().MousePos) - to_y(ImGui::GetWindowPos());
        const IdBufferPass id_buffer = IdBufferPass::create(framegraph, renderer.renderer);
        _picking_requests.emplace_back(
            picking_pass(framegraph, id_buffer, math::Vec2ui(mouse_pos)),
            _scene_view.camera(),
            mouse_pos / math::Vec2(content_size()),
            recorder.create_fence()
        );
    }

    {
        CmdTimestampPool* ts_pool = _timestamp_pools.emplace_back(std::make_unique<CmdTimestampPool>(recorder)).get();
        framegraph.render(recorder, ts_pool);
    }

    if(!output.is_null()) {
        ImGui::Image(imgui_platform()->to_ui(std::move(output)), to_im(content_size()));
    }
}



// ---------------------------------------------- UPDATE ----------------------------------------------

void EngineView::update() {
    set_is_moving_camera(false);

    if(!ImGui::IsWindowHovered() || !is_mouse_inside()) {
        _picking_valid = false;
        return;
    }

    bool focussed = ImGui::IsWindowFocused();
    if(is_clicked()) {
        ImGui::SetWindowFocus();
        focussed = true;

        if(_picking_valid &&
           _picking_result.uv.x() > 0.0f && _picking_result.uv.y() > 0.0f &&
           _picking_result.uv.x() < 1.0f && _picking_result.uv.y() < 1.0f) {

            if(_camera_controller && _camera_controller->viewport_clicked(_picking_result)) {
                // Nothing
            } else if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if(!is_dragging_gizmo()) {
                    const ecs::EntityId picked_id = _picking_result.hit() ? dynamic_cast<const EcsScene*>(&current_scene())->id_from_index(_picking_result.entity_index) : ecs::EntityId();
                    current_world().toggle_selected(picked_id, !ImGui::GetIO().KeyCtrl);
                }
            }
        }
    }

    if(focussed) {
        set_scene_view(&_scene_view);

        if(_camera_controller) {
            auto& camera = _scene_view.camera();
            _camera_controller->process_generic_shortcuts(camera);

            if(!is_dragging_gizmo() && _camera_controller->continue_moving()) {
                set_is_moving_camera(true);
                _camera_controller->update_camera(camera, content_size());
            }
        }
    }

}

void EngineView::update_scene_view() {
    if(_scene_view.scene() != &current_scene()) {
        _scene_view = SceneView(&current_scene());
    }

    const CameraSettings& settings = app_settings().camera;
    math::Vec2ui viewport_size = content_size();

    const float fov = math::to_rad(settings.fov);
    const auto proj = math::perspective(fov, float(viewport_size.x()) / float(viewport_size.y()), settings.z_near);
    _scene_view.camera().set_proj(proj);
}


void EngineView::update_picking() {
    while(!_picking_requests.is_empty()) {
        if(!_picking_requests.first().fence.is_ready()) {
            break;
        }
        const PickingRequest request = _picking_requests.pop_front();
        const shader::PickingData pick_data = request.buffer.map(MappingAccess::ReadOnly)[0];
        const math::Vec4 p = request.camera.inverse_matrix() * math::Vec4(request.uv * 2.0f - 1.0f, pick_data.depth, 1.0f);

        _picking_result.world_pos = p.to<3>() / p.w();
        _picking_result.uv = request.uv;
        _picking_result.depth = pick_data.depth;
        _picking_result.entity_index = pick_data.entity_index;

        _picking_valid = true;

        // const float dist = (_picking_result.world_pos - request.camera.position()).length();
        debug_drawer().add_primitive("debug")->add_marker(0xFF0000FF, _picking_result.world_pos);
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

        const ImVec2 cursor = ImGui::GetCursorPos();

        {
            CmdBufferRecorder recorder = create_disposable_cmd_buffer();
            draw(recorder);
            recorder.submit();
        }

        make_drop_target();

        {
            const float spacing = ImGui::GetStyle().IndentSpacing * 0.5f;
            ImGui::SetCursorPos(cursor + ImVec2(spacing, spacing));
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


        update_picking();
        update();


        if(!is_dragging_gizmo() && !_moving_camera && imgui::should_open_context_menu()) {
            ImGui::OpenPopup("##contextmenu");
        }

        if(ImGui::BeginPopup("##contextmenu")) {
            for(const EditorAction* action = all_actions(); action; action = action->next) {
                if(action->enabled && !(action->enabled()) || (action->flags & EditorAction::Contextual) != EditorAction::Contextual) {
                    continue;
                }

                if(ImGui::MenuItem(action->name.data())) {
                    action->function();
                }
            }
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
}


void EngineView::draw_toolbar_and_gizmos() {
    y_profile();

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
            reset_camera();
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

        ImGui::MenuItem("Show histogram", nullptr, &settings.debug_exposure);

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("AO")) {
        AOSettings& settings = _settings.renderer_settings.ao;

        const char* methods[] = {"MiniEngine", "RTAO", "None"};
        if(ImGui::BeginCombo("Method", methods[usize(settings.method)])) {
            for(usize i = 0; i != sizeof(methods) / sizeof(methods[0]); ++i) {
                const bool selected = usize(settings.method) == i;
                if(ImGui::Selectable(methods[i], selected)) {
                    settings.method = AOSettings::AOMethod(i);
                }
            }
            ImGui::EndCombo();
        }

        if(ImGui::BeginMenu("MiniEngine")) {
            int levels = int(settings.mini_engine.level_count);
            ImGui::SliderInt("Levels", &levels, 2, 8);
            ImGui::SliderFloat("Blur tolerance", &settings.mini_engine.blur_tolerance, 1.0f, 8.0f);
            ImGui::SliderFloat("Upsample tolerance", &settings.mini_engine.upsample_tolerance, 1.0f, 12.0f);
            ImGui::SliderFloat("Noise filter", &settings.mini_engine.noise_filter_tolerance, 0.0f, 8.0f);
            settings.mini_engine.level_count = levels;
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("RTAO")) {
            int ray = int(settings.rtao.ray_count);
            ImGui::SliderInt("Ray count", &ray, 1, 16);
            ImGui::SliderFloat("Max ray length", &settings.rtao.max_dist, 0.25f, 8.0f);
            ImGui::Separator();
            ImGui::Checkbox("Temporal stabilisation", &settings.rtao.temporal);
            ImGui::Separator();
            ImGui::SliderFloat("Filter sigma", &settings.rtao.filter_sigma, 0.0f, 8.0f);
            settings.rtao.ray_count = ray;
            ImGui::EndMenu();
        }

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
        ImGui::Checkbox("Debug tiles", &settings.debug_tiles);

        ImGui::EndMenu();
    }

    if(ImGui::BeginMenu("TAA")) {
        JitterSettings& jitter = _settings.renderer_settings.jitter;
        TAASettings& taa = _settings.renderer_settings.taa;

        ImGui::Checkbox("Enable TAA", &taa.enable);

        ImGui::Separator();

        ImGui::Checkbox("Enable clamping", &taa.use_clamping);
        ImGui::Checkbox("Enable denoise", &taa.use_denoise);

        ImGui::SliderFloat("Clamping range", &taa.clamping_range, 0.0f, 2.0f);
        ImGui::SliderFloat("Anti Flicker Strength", &taa.anti_flicker_strength, 0.0f, 8.0f);

        ImGui::Separator();

        const char* jitter_names[] = {"Weyl", "R2", "Halton23"};
        if(ImGui::BeginCombo("Jitter", jitter_names[usize(jitter.jitter)])) {
            for(usize i = 0; i != sizeof(jitter_names) / sizeof(jitter_names[0]); ++i) {
                const bool selected = usize(jitter.jitter) == i;
                if(ImGui::Selectable(jitter_names[i], selected)) {
                    jitter.jitter = JitterSettings::JitterSeq(i);
                }
            }
            ImGui::EndCombo();
        }


        ImGui::Separator();

        ImGui::SliderFloat("Jitter intensity", &jitter.jitter_intensity, 0.0f, 2.0f, "%.2f");

        ImGui::EndMenu();
    }

    ImGui::Separator();

    ImGui::MenuItem("Enable TAA", nullptr, &_settings.renderer_settings.taa.enable);
}

}

