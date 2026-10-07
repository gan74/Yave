/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "RuntimeView.h"

#include <editor/Settings.h>
#include <editor/EditorResources.h>
#include <editor/ImGuiPlatform.h>
#include <editor/utils/ui.h>

#include <yave/assets/AssetLoader.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphResourcePool.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/renderer/DefaultRenderer.h>

#include <yave/systems/AssetLoaderSystem.h>
#include <yave/systems/JoltPhysicsSystem.h>
#include <yave/systems/SceneSystem.h>
#include <yave/systems/TimeSystem.h>
#include <yave/systems/TriggerSystem.h>

#include <y/serde3/archives.h>

namespace editor {

RuntimeView::RuntimeView(io2::Buffer snapshot, const Camera& camera) :
        Widget(ICON_FA_PLAY " Runtime", ImGuiWindowFlags_MenuBar),
        _snapshot(std::move(snapshot)),
        _resource_pool(std::make_shared<FrameGraphResourcePool>()) {

    restart();
    _scene_view.camera() = camera;
}

RuntimeView::~RuntimeView() {
}

void RuntimeView::restart() {
    _world = std::make_unique<ecs::EntityWorld>();
    _world->add_system<AssetLoaderSystem>(asset_loader());
    _world->add_system<JoltPhysicsSystem>();
    _world->add_system<SceneSystem>();
    _world->add_system<TimeSystem>();
    _world->add_system<TriggerSystem>();

    _snapshot.reset();
    serde3::ReadableArchive arc(_snapshot, serde3::DeserializationFlags::DontPropagatePolyFailure);
    _world->load_state(arc).expected("Unable to load world snapshot");

    const Camera camera = _scene_view.camera();
    _scene_view = SceneView(_world->find_system<SceneSystem>()->scene());
    _scene_view.camera() = camera;
}

void RuntimeView::update_camera() {
    const CameraSettings& settings = app_settings().camera;
    const math::Vec2ui viewport_size = content_size();
    _scene_view.camera().set_proj(math::perspective(math::to_rad(settings.fov), float(viewport_size.x()) / float(viewport_size.y()), settings.z_near));

    if(_camera_controller && ImGui::IsWindowHovered() && _camera_controller->continue_moving()) {
        _camera_controller->update_camera(_scene_view.camera(), viewport_size);
    }
}

void RuntimeView::draw() {
    DstTexture output;

    FrameGraph framegraph(_resource_pool);
    const DefaultRenderer renderer = DefaultRenderer::create(framegraph, _scene_view, content_size());

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("ImGui texture pass");
    const auto output_image = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, content_size());
    builder.add_input_usage(output_image, ImageUsage::TransferSrcBit);
    builder.add_color_output(output_image);
    builder.add_uniform_input(renderer.final);
    builder.set_render_func([=, &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        {
            auto render_pass = recorder.bind_framebuffer(self->framebuffer());
            render_pass.bind_material_template(resources()[EditorResources::OETFMaterialTemplate], self->descriptor_set());
            render_pass.draw_array(3);
        }
        const auto& src = self->resources().image_base(output_image);
        output = DstTexture(src.format(), src.image_size().to<2>());
        recorder.copy(src, output);
    });

    {
        CmdBufferRecorder recorder = create_disposable_cmd_buffer();
        framegraph.render(recorder);
        recorder.submit();
    }

    if(!output.is_null()) {
        ImGui::Image(imgui_platform()->to_ui(std::move(output)), to_im(content_size()));
    }
}

void RuntimeView::on_gui() {
    if(ImGui::BeginMenuBar()) {
        if(ImGui::MenuItem(ICON_FA_REDO " Restart")) {
            restart();
        }

        TimeSystem* time = _world->find_system<TimeSystem>();
        const bool paused = time->time_scale() <= 0.0f;
        if(ImGui::MenuItem(paused ? ICON_FA_PLAY : ICON_FA_PAUSE)) {
            time->set_time_scale(paused ? 1.0f : 0.0f);
        }

        ImGui::EndMenuBar();
    }

    _world->tick(editor_job_system());
    _world->process_deferred_changes();

    if(ImGui::BeginChild("##view")) {
        update_camera();
        draw();
    }
    ImGui::EndChild();
}

}
