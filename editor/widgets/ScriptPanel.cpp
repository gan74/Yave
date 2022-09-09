/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "ScriptPanel.h"

#include <editor/EditorWorld.h>
#include <editor/widgets/FileBrowser.h>

#include <yave/components/ScriptWorldComponent.h>

#include <yave/utils/FileSystemModel.h>

#include <external/imgui/yave_imgui.h>

#include <y/io2/File.h>

namespace editor {

ScriptPanel::ScriptPanel() : Widget(ICON_FA_CODE " Scripts") {
}

void ScriptPanel::on_gui() {
    ScriptWorldComponent* scripts = current_world().get_or_add_world_component<ScriptWorldComponent>();

    ImGui::Text("%u scripts", u32(scripts->scripts().size()));

    if(ImGui::Button(ICON_FA_PLUS " Add")) {
        FileBrowser* browser = add_child_widget<FileBrowser>(FileSystemModel::local_filesystem());
        browser->set_selection_filter(false, "*.lua");
        browser->set_selected_callback([](const auto& filename) {
            if(auto r = io2::File::read_text_file(filename)) {
                current_world().get_or_add_world_component<ScriptWorldComponent>()->scripts().emplace_back(std::move(r.unwrap()));
            }
            return true;
        });
    }

    ImGui::SameLine();

    if(ImGui::Button(ICON_FA_TRASH " Clear")) {
        scripts->scripts().clear();
    }
}

}

