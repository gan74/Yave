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
#include "CameraController.h"

#include <editor/widgets/AssetSelector.h>
#include <editor/context/EditorContext.h>
#include <editor/context/Picker.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphResourcePool.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/renderer/DefaultRenderer.h>
#include <yave/utils/color.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

static bool is_clicked() {
    return ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2);
}

static auto standard_resolutions() {
    static std::array<std::pair<const char*, math::Vec2ui>, 3> resolutions = {{
        {"1080p", {1920, 1080}},
        {"1440p", {2560, 1440}},
        {"4k",    {3840, 2160}},
    }};

    return resolutions;
}


EngineView::EngineView(ContextPtr cptr) :
        Widget(ICON_FA_DESKTOP " Engine View", ImGuiWindowFlags_MenuBar),
        ContextLinked(cptr),
        _resource_pool(std::make_shared<FrameGraphResourcePool>(device())),
        _scene_view(&context()->world()),
        _camera_controller(std::make_unique<HoudiniCameraController>(context())),
        _gizmo(context(), &_scene_view) {
}

EngineView::~EngineView() {
    context()->remove_scene_view(&_scene_view);
}

void EngineView::before_paint() {
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, math::Vec4(0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, math::Vec4(0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, math::Vec2(2.0f, 0.0f));
}

void EngineView::after_paint() {
    ImGui::PopStyleColor(2);
    // poped during menu rendering
    // ImGui::PopStyleVar(1);
}

void EngineView::draw(CmdBufferRecorder& recorder) {
    TextureView* output = nullptr;
    FrameGraph graph(_resource_pool);

    const math::Vec2ui output_size = _resolution < 0
        ? content_size()
        : standard_resolutions()[_resolution].second;

    const EditorRenderer renderer = EditorRenderer::create(context(), graph, _scene_view, output_size, _settings);

    {
        const Texture& white = *device_resources(graph.device())[DeviceResources::WhiteTexture];

        FrameGraphPassBuilder builder = graph.add_pass("ImGui texture pass");

        const auto output_image = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, output_size);
        const auto buffer = builder.declare_typed_buffer<u32>(1);

        const auto gbuffer = renderer.renderer.gbuffer;
        builder.add_image_input_usage(output_image, ImageUsage::TextureBit);
        builder.add_color_output(output_image);
        builder.add_uniform_input(buffer);
        builder.add_uniform_input(renderer.color, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.color, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input_with_default(renderer.renderer.ssao.ao, Descriptor(white), 0, PipelineStage::FragmentBit);
        builder.map_update(buffer);
        builder.set_render_func([=, index = u32(_view), &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
                auto out = std::make_unique<TextureView>(self->resources().image<ImageUsage::TextureBit>(output_image));
                output = out.get();
                recorder.keep_alive(std::move(out));

                TypedMapping<u32> mapping = self->resources().mapped_buffer(buffer);
                mapping[0] = index;

                auto render_pass = recorder.bind_framebuffer(self->framebuffer());
                const auto* material = context()->resources()[EditorResources::EngineViewMaterialTemplate];
                render_pass.bind_material(material, {self->descriptor_sets()[0]});

                VkDrawIndirectCommand command = {};
                {
                    command.vertexCount = 6;
                    command.instanceCount = 1;
                }
                render_pass.draw(command);
            });
    }

    if(!_disable_render) {
        std::move(graph).render(recorder);
    }

    if(output) {
        ImGui::GetWindowDrawList()->AddImage(output,
            position() + math::Vec2(ImGui::GetWindowContentRegionMin()),
            position() + math::Vec2(ImGui::GetWindowContentRegionMax()));
    }

}

void EngineView::paint(CmdBufferRecorder& recorder) {
    y_profile();

    update_proj();

    draw_menu_bar();
    draw(recorder);
    _gizmo.draw();

    update();
}

void EngineView::update_proj() {
    const CameraSettings& settings = context()->settings().camera();
    math::Vec2ui viewport_size = content_size();

    const float fov = math::to_rad(settings.fov);
    const auto proj = math::perspective(fov, float(viewport_size.x()) / float(viewport_size.y()), settings.z_near);
    _scene_view.camera().set_proj(proj);
}

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
        context()->set_scene_view(&_scene_view);
    }


    if(hovered && !_gizmo.is_dragging() && _camera_controller) {
        auto& camera = _scene_view.camera();
        _camera_controller->process_generic_shortcuts(camera);
        if(focussed) {
            _camera_controller->update_camera(camera, content_size());
        }
    }
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

    const PickingResult picking_data = Picker(context()).pick_sync(_scene_view, uv, viewport_size);
    if(_camera_controller && _camera_controller->viewport_clicked(picking_data)) {
        // event has been eaten by the camera controller, don't proceed further
        _gizmo.set_allow_drag(false);
        return;
    }

    if(ImGui::IsMouseClicked(0)) {
        if(!_gizmo.is_dragging()) {
            ecs::EntityId picked_id = picking_data.hit() ? context()->world().id_from_index(picking_data.entity_index) : ecs::EntityId();
            context()->selection().set_selected(picked_id);
        }
    }
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

        int levels = settings.level_count;

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

        ImGui::SliderFloat("Power", &settings.bloom_power, 0.0f, 100.0f, "%.3f", 10.0f);
        ImGui::SliderFloat("Threshold", &settings.bloom_threshold, 0.0f, 1.0f);

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
        ImGui::PopStyleVar(1);

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
            ImGui::TextUnformatted(standard_resolutions()[_resolution].first);
        }

        ImGui::EndMenuBar();
    }
}

void EngineView::draw_gizmo_tool_bar() {
    ImGui::Separator();

    auto gizmo_mode = _gizmo.mode();
    auto gizmo_space = _gizmo.space();
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

    if(is_focussed()) {
        const UiSettings& settings = context()->settings().ui();
        if(ImGui::IsKeyReleased(int(settings.change_gizmo_mode))) {
            gizmo_mode = Gizmo::Mode(!usize(gizmo_mode));
        }
        if(ImGui::IsKeyReleased(int(settings.change_gizmo_space))) {
            gizmo_space = Gizmo::Space(!usize(gizmo_space));
        }
    }
    _gizmo.set_mode(gizmo_mode);
    _gizmo.set_space(gizmo_space);
}

}

