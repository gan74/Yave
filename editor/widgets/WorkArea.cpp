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

#include "WorkArea.h"

#include <editor/UiManager.h>

namespace editor {

WorkArea::WorkArea(std::unique_ptr<Workspace> workspace) : Widget(workspace->name()), _workspace(std::move(workspace)) {
}

WorkArea::~WorkArea() {
}

Workspace* WorkArea::workspace() {
    return _workspace.get();
}

const Workspace* WorkArea::workspace() const {
    return _workspace.get();
}

void WorkArea::on_gui() {
    draw_dockspace(ImGuiDockNodeFlags_None);
}

void WorkArea::on_inactive_gui() {
    draw_dockspace(ImGuiDockNodeFlags_KeepAliveOnly);
}

void WorkArea::draw_dockspace(ImGuiDockNodeFlags flags) {
    ImGuiWindowClass window_class;
    window_class.ClassId = _workspace->workspace_id();
    window_class.DockingAllowUnclassed = false;

    const ImGuiID dockspace_id = ImGui::GetID("##workarea_dock");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), flags, &window_class);
}

bool WorkArea::should_keep_alive() const {
    return true;
}

WorkArea* find_work_area() {
    for(const auto& widget : ui().widgets()) {
        if(WorkArea* area = dynamic_cast<WorkArea*>(widget.get())) {
            return area;
        }
    }
    return nullptr;
}

}
