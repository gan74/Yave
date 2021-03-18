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

#include "FileBrowser.h"

#include <editor/utils/ui.h>

#include <yave/utils/FileSystemModel.h>

#include <y/utils/sort.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>


namespace editor {

template<usize N>
static void to_buffer(std::array<char, N>& buffer, std::string_view str) {
    const usize len = std::min(buffer.size() - 1, str.size());
    std::copy_n(str.begin(), len, buffer.data());
    buffer[len] = 0;
}

FileBrowser::FileBrowser(const FileSystemModel* fs) : FileSystemView(fs) {
    path_changed();
}

void FileBrowser::set_selection_filter(bool dirs, std::string_view exts) {
    _dirs = dirs;
    for(char c : exts) {
        if(c == ';' || _extensions.is_empty()) {
            _extensions.emplace_back();
        } else if(c != '*') {
            _extensions.last().push_back(c);
        }
    }
    std::sort(_extensions.begin(), _extensions.end());
    // refresh();
}


void FileBrowser::path_changed() {
    to_buffer(_path_buffer, path());
}

core::Result<core::String> FileBrowser::entry_icon(const core::String& name, EntryType type) const {
    if(type == EntryType::Directory) {
        return core::Ok(core::String(ICON_FA_FOLDER));
    }
    if(_extensions.is_empty()) {
        return core::Err();
    }
    if(has_valid_extension(name)) {
        return core::Ok(core::String(ICON_FA_FILE_ALT));
    }
    return core::Err();
}

void FileBrowser::entry_clicked(const Entry& entry) {
    if(!_dirs) {
        if(entry.type == EntryType::File) {
            done(entry_full_name(entry));
        }
    }
    FileSystemView::entry_clicked(entry);
}

bool FileBrowser::has_valid_extension(std::string_view filename) const {
    if(_extensions.is_empty()) {
        return false;
    }
    const auto ext = filesystem()->extention(filename);
    return std::binary_search(_extensions.begin(), _extensions.end(), ext);
}

bool FileBrowser::done(const core::String& filename) {
    y_profile();
    const bool valid_dir = filesystem()->is_directory(filename).unwrap_or(false);
    const bool valid_file = has_valid_extension(filename) && filesystem()->is_file(filename).unwrap_or(false);

    bool changed = true;
    if((valid_dir && _dirs) || valid_file) {
        set_visible(!_callbacks.selected(filename));
    } else if(valid_dir) {
        set_path(filename);
    } else {
        changed = false;
    }

    update();
    return changed;
}

void FileBrowser::cancel() {
    set_visible(!_callbacks.canceled());
}

core::String FileBrowser::full_path() const {
    return filesystem()->join(path(), _name_buffer.data());
}

void FileBrowser::on_gui() {
    static constexpr float button_width = 75.0f;

    {
        const float button_inner_width = button_width - (ImGui::GetStyle().FramePadding.x * 2.0f);
        const float buttons_size = _extensions.is_empty() ? (button_width * 2) : button_width;
        ImGui::SetNextItemWidth(-buttons_size);
        if(ImGui::InputText("##path", _path_buffer.data(), _path_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            set_path(_path_buffer.data());
        }

        ImGui::SameLine();
        if(ImGui::Button("Ok", ImVec2(button_inner_width, 0.0f))) {
            done(full_path());
        }

        if(!_extensions.is_empty()) {
            ImGui::SetNextItemWidth(-button_width);
            if(ImGui::InputText("##filename", _name_buffer.data(), _name_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                if(done(full_path())) {
                    _name_buffer[0] = 0;
                }
            }
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(button_inner_width, 0.0f))) {
            cancel();
        }
    }

    FileSystemView::on_gui();
}

}

