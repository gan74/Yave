/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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




namespace editor {

FileBrowser::FileBrowser(const FileSystemModel* fs) : Widget("File Browser"), _filesystem_view(fs) {
    _filesystem_view.set_filter_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) {
        if(type == FileSystemModel::EntryType::Directory) {
            return true;
        }
        if(_extensions.is_empty()) {
            return false;
        }
        return has_valid_extension(full_name);
    });

    _filesystem_view.set_clicked_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) {
        if(!_dirs) {
            if(type == FileSystemModel::EntryType::File) {
                done(full_name);
                return true;
            }
        }
        return false;
    });
}

const FileSystemModel* FileBrowser::filesystem() const {
    return _filesystem_view.filesystem();
}

void FileBrowser::set_selection_filter(std::string_view exts, FilterFlags flags) {
    _dirs = (flags & FilterFlags::IncludeDirs) == FilterFlags::IncludeDirs;
    _allow_new = (flags & FilterFlags::AllowNewFiles) == FilterFlags::AllowNewFiles;

    for(char c : exts) {
        if(c == ';' || _extensions.is_empty()) {
            _extensions.emplace_back();
        } else if(c != '*') {
            _extensions.last().push_back(c);
        }
    }
    std::sort(_extensions.begin(), _extensions.end());
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
    const bool valid_file = /*has_valid_extension(filename) &&*/ (_allow_new || filesystem()->is_file(filename).unwrap_or(false));

    bool changed = true;
    if((valid_dir && _dirs) || valid_file) {
        set_visible(!_callbacks.selected(filename));
    } else if(valid_dir) {
        _filesystem_view.set_path(filename);
    } else {
        changed = false;
    }

    _filesystem_view.refresh();
    return changed;
}

void FileBrowser::cancel() {
    set_visible(!_callbacks.canceled());
}

core::String FileBrowser::full_path() const {
    return filesystem()->join(_filesystem_view.path(), _name_buffer.data());
}

void FileBrowser::on_gui() {
    static constexpr float button_width = 75.0f;

    {
        const float button_inner_width = button_width - (ImGui::GetStyle().FramePadding.x * 2.0f);
        const float buttons_size = _extensions.is_empty() ? (button_width * 2) : button_width;
        ImGui::SetNextItemWidth(-buttons_size);
        if(imgui::text_input("##path", _path_buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            _filesystem_view.set_path(_path_buffer);
        }

        ImGui::SameLine();
        if(ImGui::Button("Ok", ImVec2(button_inner_width, 0.0f))) {
            done(full_path());
        }

        if(!_extensions.is_empty()) {
            ImGui::SetNextItemWidth(-button_width);
            if(imgui::text_input("##filename", _name_buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
                if(done(full_path())) {
                    _name_buffer.make_empty();
                }
            }
        }

        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(button_inner_width, 0.0f))) {
            cancel();
        }
    }

    _filesystem_view.draw_gui_inside();
}

}

