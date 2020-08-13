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

#include <y/io2/File.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <imgui/yave_imgui.h>

namespace editor {

core::String clean_type_name(std::string_view name) {
    usize start = 0;
    for(usize i = 0; i != name.size(); ++i) {
        switch(name[i]) {
            case ':':
                start = i + 1;
            break;

            default:
            break;
        }
    }

    core::String clean;
    for(usize i = start; i != name.size(); ++i) {
        if(std::isupper(name[i])) {
            clean.push_back(' ');
        }
        clean.push_back(name[i]);
    }

    return clean;
}

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

    {
        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar | flags);
        ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    }

    ImGui::PopStyleVar(3);
}


static void imgui_end_frame() {
    y_profile();

    ImGui::End();
    ImGui::Render();
}

static void imgui_setup() {
    static bool setup = false;
    if(!setup) {
        setup = true;

        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = "editor.ini";
        ImGui::GetIO().LogFilename = "editor_logs.txt";
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().ConfigDockingWithShift = false;
        //ImGui::GetIO().ConfigResizeWindowsFromEdges = true;

        if(io2::File::open("../editor.ini").is_ok()) {
            ImGui::GetIO().IniFilename = "../editor.ini";
        }
    }
}

UiSystem::UiSystem(ContextPtr ctx, MainWindow& window) :
        ecs::System("UiSystem"),
        _window(&window),
        _renderer(std::make_unique<ImGuiRenderer>(ctx)) {

    imgui_setup();
}

UiSystem::~UiSystem() {
}

void UiSystem::tick(ecs::EntityWorld& world) {
    {
        for(ecs::EntityId id : world.recently_added<UiComponent>()) {
            if(UiWidgetBase* widget = world.component<UiComponent>(id)->widget.get()) {
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

        paint_menu(world);
        paint_widgets(world, recorder);

        imgui_end_frame();
    }

    {
        y_profile_zone("render");
        Framebuffer framebuffer(frame.image_view.device(), {frame.image_view});
        RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
        _renderer->render(pass, frame);
    }
    _window->present(recorder, frame);
}

bool UiSystem::should_delete(const ecs::EntityWorld& world, const UiComponent* component) const {
    return !component ||
           !component->widget ||
           !component->widget->_visible ||
           (component->parent.is_valid() && !world.has<UiComponent>(component->parent));
}

void UiSystem::paint_menu(ecs::EntityWorld& world) {
    if(ImGui::BeginMenuBar()) {
        if(ImGui::BeginMenu("View")) {
            for(const auto* poly_base = UiWidgetBase::_y_serde3_poly_base.first; poly_base; poly_base = poly_base->next) {
                const core::String name = clean_type_name(poly_base->name);
                if(ImGui::MenuItem(name.data())) {
                    if(auto widget = poly_base->create()) {
                        UiComponent::create_widget(world, std::move(widget));
                    } else {
                        log_msg(fmt("Unable to create %", poly_base->name), Log::Error);
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void UiSystem::paint_widgets(ecs::EntityWorld& world, CmdBufferRecorder& recorder) {
    core::Vector<ecs::EntityId> to_remove;

    Y_TODO(Defer component addition so we dont have to copy here)
    core::Vector<ecs::EntityId> ids(world.component_ids<UiComponent>());
    for(ecs::EntityId id : ids) {
        y_debug_assert(id.is_valid());
        UiComponent* comp = world.component<UiComponent>(id);
        if(should_delete(world, comp)) {
            to_remove << id;
            continue;
        }
        paint_widget(world, recorder, comp->widget.get());
    }

    Y_TODO(check if entity has other components)
    for(ecs::EntityId id : to_remove) {
        world.remove_entity(id);
    }
}

void UiSystem::paint_widget(ecs::EntityWorld& world, CmdBufferRecorder& recorder, UiWidgetBase* widget) {
    y_debug_assert(widget);

    const bool b = ImGui::Begin(widget->_title_with_id.begin(), &widget->_visible, 0);
    {
        widget->update_attribs();
        if(b) {
            widget->paint(world, recorder);
        }
    }
    ImGui::End();
}

}
