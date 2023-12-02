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

#include "Renamer.h"

#include <yave/utils/FileSystemModel.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <editor/utils/ui.h>

namespace editor {

/*static std::string_view decompose_path(std::string_view full_name, std::string_view name) {
    y_debug_assert(full_name.size() >= name.size());
    y_debug_assert(full_name.substr(full_name.size() - name.size()) == name);
    return full_name.substr(0, full_name.size() - name.size());
}*/



Renamer::Renamer(std::string_view name, std::function<bool(std::string_view)> callback) :
        Widget("Rename", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking),
        _name(name),
        _name_buffer(name),
        _callback(callback) {

    set_modal(true);
}

const core::String& Renamer::original_name() const {
    return _name;
}

void Renamer::on_gui() {
    ImGui::Text("Rename: \"%s\"", _name.data());

    if(imgui::text_input("##name", _name_buffer, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("Ok")) {
        if(_callback(_name_buffer)) {
            close();
            refresh_all();
        } else {
            log_msg("Unable to rename", Log::Error);
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Cancel")) {
        close();
    }
}

static bool rename_file(const FileSystemModel* fs, std::string_view old_name, std::string_view new_name) {
    const auto path = fs->parent_path(old_name);
    const auto full_new_name = path.map([&](auto&& p) { return fs->join(p, new_name); });
    return full_new_name && fs->rename(old_name, full_new_name.unwrap());
}

FileRenamer::FileRenamer(const FileSystemModel* fs, core::String filename) :
        Renamer(filename, [=](std::string_view new_name) { return rename_file(fs, filename, new_name); }) {
}


}

