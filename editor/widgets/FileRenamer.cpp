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

#include "FileRenamer.h"

#include <yave/utils/FileSystemModel.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

/*static std::string_view decompose_path(std::string_view full_name, std::string_view name) {
    y_debug_assert(full_name.size() >= name.size());
    y_debug_assert(full_name.substr(full_name.size() - name.size()) == name);
    return full_name.substr(0, full_name.size() - name.size());
}*/


FileRenamer::FileRenamer(const FileSystemModel* fs, core::String filename) :
        Widget("Rename", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking),
        _filesystem(fs),
        _filename(std::move(filename)),
        _name(fs->filename(_filename)) {

    const usize size = std::min(_new_name.size(), _name.size() + 1);
    std::copy_n(_name.begin(), size, _new_name.begin());
}

void FileRenamer::on_gui() {
    ImGui::Text("Rename: \"%s\"", _name.data());

    if(ImGui::InputText("", _new_name.data(), _new_name.size(), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("Ok")) {
        const auto path = _filesystem->parent_path(_filename);
        const auto full_new_name = path.map([this](auto&& p) { return _filesystem->join(p, _new_name.data()); });
        if(full_new_name && _filesystem->rename(_filename, full_new_name.unwrap())) {
            close();
            refresh_all();
        } else {
            log_msg("Unable to rename file.", Log::Error);
        }
    }

    ImGui::SameLine();
    if(ImGui::Button("Cancel")) {
        close();
    }
}

}

