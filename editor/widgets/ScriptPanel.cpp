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

#include "ScriptPanel.h"

#include <editor/EditorWorld.h>
#include <editor/widgets/FileBrowser.h>
#include <editor/utils/ui.h>

#include <yave/components/ScriptWorldComponent.h>

#include <yave/utils/FileSystemModel.h>



#include <y/io2/File.h>

namespace editor {

static void edit_script(ScriptWorldComponent::Script& script) {
    const core::String temp_dir = core::String(std::tmpnam(nullptr)) + "/";
    const core::String file_name = temp_dir + script.name + ".lua";

    const FileSystemModel* fs = FileSystemModel::local_filesystem();
    fs->create_directory(temp_dir).ignore();

    auto fill_file = [&] {
        if(auto r = io2::File::create(file_name)) {
            if(r.unwrap().write(script.code.data(), script.code.size())) {
                return true;
            }
        }
        return false;
    };

    auto read_file = [&] {
        if(auto r = io2::File::read_text_file(file_name)) {
            std::swap(script.code, r.unwrap());
            return true;
        }
        return false;
    };

    if(!fill_file()) {
        log_msg("Unable to create script file", Log::Error);
        return;
    }

    y_defer(fs->remove(file_name).ignore());

    std::system(file_name.data());

    if(!read_file()) {
        log_msg("Unable to read script file", Log::Error);
        return;
    }

    log_msg("Script reloaded");
}


ScriptPanel::ScriptPanel() : Widget(ICON_FA_CODE " Scripts"), _buffer(32 * 1024) {
}

void ScriptPanel::on_gui() {
    ScriptWorldComponent* script_component = current_world().get_or_add_world_component<ScriptWorldComponent>();
    auto& scripts = script_component->scripts();

    for(usize i = 0; i != scripts.size(); ++i) {
        if(ImGui::Button(fmt_c_str(ICON_FA_EDIT "##%", i))) {
            edit_script(scripts[i]);
        }
        ImGui::SameLine();
        if(ImGui::Button(fmt_c_str(ICON_FA_TRASH "##%", i))) {
            scripts.erase(scripts.begin() + i);
            break;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(scripts[i].name.data());
    }

    if(ImGui::Button(ICON_FA_PLUS " Add")) {
        FileBrowser* browser = add_child_widget<FileBrowser>(FileSystemModel::local_filesystem());
        browser->set_selection_filter(false, "*.lua");
        browser->set_selected_callback([](const auto& filename) {
            if(auto r = io2::File::read_text_file(filename)) {
                const core::String name = FileSystemModel::local_filesystem()->filename(filename);
                auto& scripts = current_world().get_or_add_world_component<ScriptWorldComponent>()->scripts();
                scripts.emplace_back(ScriptWorldComponent::Script{
                    name.sub_str(0, name.size() - 4),
                    std::move(r.unwrap())
                });
            } else {
                log_msg("Unable to read file", Log::Error);
            }
            return true;
        });
    }


    {
        if(_result && _result->done) {
            if(_result->result.is_error()) {
                log_msg(fmt("Lua error: %", _result->result.error()), Log::Error);
                _error = _result->result.error();
            }
            _result = nullptr;
        }


        const float text_height = imgui::text_line_count(_error) * imgui::button_height();
        const math::Vec2 avail_size = math::Vec2(ImGui::GetContentRegionAvail()) - math::Vec2(0.0f, imgui::button_height() + text_height);
        ImGui::InputTextMultiline("##console", _buffer.data(), _buffer.size(), avail_size);

        if(!_error.is_empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
            ImGui::InputTextMultiline("###error", _error.data(), _error.size(), math::Vec2(avail_size.x(), text_height), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor();
        }

        if(ImGui::Button(ICON_FA_PLAY " Run")) {
            _error.make_empty();
            _result = script_component->run_once(_buffer.data());
        }
    }
}

}

