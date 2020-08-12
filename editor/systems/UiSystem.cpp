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
#include "UiSystem.h"

#include <editor/MainWindow.h>
#include <editor/ui/UiElement.h>
#include <editor/components/UiComponent.h>
#include <editor/ui/ImGuiRenderer.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/swapchain/Swapchain.h>
#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/ecs/EntityWorld.h>
#include <yave/graphics/utils.h>

#include <imgui/yave_imgui.h>

namespace editor {

static void imgui_begin_frame(const math::Vec2& size, double dt) {
    y_profile();

    ImGui::GetIO().DeltaTime = float(dt);
    ImGui::GetIO().DisplaySize = size;

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus;


    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar | flags);

    ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::PopStyleVar(3);
}


static void imgui_end_frame() {
    y_profile();

    ImGui::End();
    ImGui::Render();
}

UiSystem::UiSystem(ContextPtr ctx, MainWindow& window) :
        ecs::System("UiSystem"),
        _window(&window),
        _renderer(std::make_unique<ImGuiRenderer>(ctx)) {
}

void UiSystem::tick(ecs::EntityWorld& world) {
    {
        for(ecs::EntityId id : world.recently_added<UiComponent>()) {
            if(UiWidget* widget = world.component<UiComponent>(id)->widget.get()) {
                widget->_entity_id = id;
            }
        }
    }

    Swapchain* swapchain = _window->swapchain();
    if(!swapchain || !swapchain->is_valid()) {
        return;
    }

    FrameToken frame = swapchain->next_frame();
    CmdBufferRecorder recorder = create_disposable_cmd_buffer(_window->device());

    {
        imgui_begin_frame(_window->size(), _frame_timer.reset().to_secs());
        core::Vector<ecs::EntityId> to_remove;

        Y_TODO(Defer component addition so we dont have to copy here)
        core::Vector<ecs::EntityId> ids(world.component_ids<UiComponent>());
        for(ecs::EntityId id : ids) {
            UiComponent* comp = world.component<UiComponent>(id);
            if(!comp || !comp->widget || !world.has<UiComponent>(comp->parent)) {
                to_remove << id;
                continue;
            }
            comp->widget->paint(world, recorder);
        }

        Y_TODO(check if entity has other components)
        for(ecs::EntityId id : to_remove) {
            world.remove_entity(id);
        }
        imgui_end_frame();
    }

    {
        y_profile_zone("render");
        Framebuffer framebuffer(frame.image_view.device(), {frame.image_view});
        RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
        _renderer->render(pass, frame);
        _window->present(recorder, frame);
    }
}

}
