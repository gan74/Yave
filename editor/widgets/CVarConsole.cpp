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

#include "CVarConsole.h"

#include <editor/utils/ui.h>
#include <editor/Settings.h>

#include <y/utils/name.h>

#include <external/imgui/yave_imgui.h>

#include <algorithm>
#include <string>


namespace editor {

template<typename T>
static constexpr bool is_printable_v =
    std::is_arithmetic_v<T> ||
    //std::is_enum_v<T> ||
    (std::is_constructible_v<std::string_view, T> && std::is_constructible_v<T, std::string_view>);


template<typename T>
static bool try_parse(std::string_view str, T& t) {
    unused(str, t);
    if constexpr(std::is_arithmetic_v<T>) {
        try {
            usize pos = 0;
            const double val = std::stod(std::string(str), &pos); // Why???
            if(pos != str.size()) {
                return false;
            }
            t = T(val);
            return true;
        } catch(...) {
            return false;
        }
    } else if constexpr(std::is_constructible_v<std::string_view>) {
        t = T(str);
        return true;
    }
    return false;
}

template<typename T, typename C, typename F>
static void collect_var(C& vars, const core::String& parent, std::string_view name, F&& getter) {
    unused(vars, parent, name, getter);

    const core::String full_name = parent + "." + name;
    if constexpr(is_printable_v<T>) {
        vars.emplace_back(
            full_name,
            [=]{ return fmt("%", getter()); },
            [=](std::string_view str) { return try_parse(str, getter()); }
        );
    } else if constexpr(is_iterable_v<T>) {
        if constexpr(is_printable_v<T::value_type>) {
            vars.emplace_back(
                full_name,
                [=]{ return fmt("%", getter()); },
                [=](std::string_view) {
                    /* not supported */
                    return false;
                }
            );
        }
    }
}

template<typename C, typename F>
static void explore(C& vars, const core::String& parent, std::string_view name, F&& getter) {
    using result_type = remove_cvref_t<decltype(getter())>;

    collect_var<result_type>(vars, parent, name, getter);

    auto& obj = getter();
    char* ptr = reinterpret_cast<char*>(&obj);

    reflect::explore_members(obj, [=, full = parent + "." + name, &vars](std::string_view name, auto& e) {
        // This will break if we run into polymorphism or virtual bases, static_assert on that...
        using elem_type = remove_cvref_t<decltype(e)>;
        const usize offset = reinterpret_cast<char*>(&e) - ptr;
        explore(vars, full, name, [=]() -> elem_type& {
            auto&& p = getter();
            char* base_ptr = reinterpret_cast<char*>(&p);
            return *reinterpret_cast<elem_type*>(base_ptr + offset);
        });
    });
}

CVarConsole::CVarConsole() : Widget(ICON_FA_STREAM " CVars") {
    explore(_cvars, "", "settings", app_settings);
    set_pattern("");
}


void CVarConsole::set_pattern(std::string_view str) {
    _search_pattern = str;
    _search_pattern.grow(256, 0);
}

void CVarConsole::on_gui() {
    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
    ImGui::TextUnformatted(ICON_FA_EXCLAMATION_TRIANGLE " Don't use this if you don't know what you are doing!");
    ImGui::PopStyleColor();

    const float footer_height = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ChildBorderSize;
    if(ImGui::BeginChild("##CVarConsole", ImVec2(0.0f, -footer_height), true)) {

        for(const auto& msg : _msgs) {
            ImGui::Text("# %s", msg.command.data());
            ImGui::PushStyleColor(ImGuiCol_Text, msg.is_error ? 0xFF0000FF : 0xFFFF8000);
            ImGui::TextUnformatted(msg.result.data());
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();

    if(imgui::search_bar(_search_pattern.data(), _search_pattern.size())) {
        process_command(_search_pattern.data());
        _search_pattern[0] = 0;
    }

    if(imgui::begin_suggestion_popup()) {
        const std::string_view pattern = _search_pattern.data();
        for(const auto& var : _cvars) {
            if(var.full_name.find(pattern) != var.full_name.end()) {
                const char* name = fmt_c_str("% = %", var.full_name, var.to_string());
                if(imgui::suggestion_item(name)) {
                    set_pattern(var.full_name);
                }
            }
        }
        imgui::end_suggestion_popup();
    }
}

void CVarConsole::process_command(std::string_view command) {
    command = core::trim(command);
    const usize eq = command.find('=');

    if(eq != std::string_view::npos) {
        const std::string_view name = core::trim_right(command.substr(0, eq));
        for(const auto& var : _cvars) {
            if(var.full_name == name) {
                const std::string_view value = core::trim_left(command.substr(eq + 1));
                if(!var.from_string(value)) {
                    _msgs.emplace_back(command, fmt("Unable to parse value '%'", value), true);
                } else {
                    _msgs.emplace_back(command, var.to_string(), false);
                }
                return;
            }
        }
    } else {
        for(const auto& var : _cvars) {
            if(var.full_name == command) {
                _msgs.emplace_back(command, var.to_string(), false);
                return;
            }
        }
    }
    _msgs.emplace_back(command, fmt("\"%\" not found", command), true);
}


}

