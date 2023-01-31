
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

#include "CVarConsole.h"

#include <editor/utils/ui.h>
#include <editor/Settings.h>

#include <y/utils/name.h>



#include <algorithm>
#include <string>
#include <regex>


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
            [=](std::string_view str) { return try_parse(str, getter()); },
            ct_type_name<T>(), "", false, false
        );
    } else if constexpr(is_iterable_v<T>) {
        if constexpr(is_printable_v<typename T::value_type>) {
            vars.emplace_back(
                full_name,
                [=]{ return fmt("%", getter()); },
                [=](std::string_view) { /* not supported */ return false; },
                ct_type_name<T>(), "", false, false
            );
        }
    }
}

template<typename C, typename F>
static void explore(C& vars, const core::String& parent, std::string_view name, F&& getter) {
    using result_type = remove_cvref_t<decltype(getter())>;

    collect_var<result_type>(vars, parent, name, getter);

    reflect::explore_members<result_type>([&, full = parent + "." + name](std::string_view name, auto member) mutable {
        using member_type = remove_cvref_t<decltype(std::declval<result_type>().*member)>;
        explore(vars, full, name, [getter, member]() -> member_type& {
            return getter().*member;
        });
    });
}




CVarConsole::CVarConsole() : Widget(ICON_FA_STREAM " CVars") {
    explore(_cvars, "", "settings", app_settings);

    for(auto& var : _cvars) {
        var.default_value = var.to_string();
    }
}

void CVarConsole::on_gui() {
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(ICON_FA_FILTER " ").x);
    ImGui::InputText(ICON_FA_FILTER "##filter", _search_pattern.data(), _search_pattern.size());
    const bool search_empty = !_search_pattern[0];

    const ImGuiTableFlags table_flags =
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_RowBg;

    if(ImGui::BeginChild("##entities", ImVec2(), true)) {
        if(ImGui::BeginTable("##cvars", 3, table_flags)) {
            ImGui::TableSetupColumn("##name");
            ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##restore", ImGuiTableColumnFlags_NoResize);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));
            y_defer(ImGui::PopStyleVar());

            const std::regex regex = search_empty ? std::regex() : std::regex(_search_pattern.data(), std::regex::icase);
            int id = 0;
            for(auto& var : _cvars) {
                if(!search_empty && !std::regex_search(var.full_name.data(), regex)) {
                    continue;
                }
                ImGui::PushID(id++);

                imgui::table_begin_next_row();

                {
                    ImGui::TextUnformatted(var.full_name.data());
                }

                ImGui::TableSetColumnIndex(1);

                {
                    bool pop_color = false;
                    if(var.error) {
                        ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
                        pop_color = true;
                    } else if(var.modified) {
                        ImGui::PushStyleColor(ImGuiCol_Text, math::Vec4(0.3f, 0.3f, 1.0f, 1.0f));
                        pop_color = true;
                    }

                    const std::string_view view = var.to_string();
                    std::copy_n(view.data(), std::max(view.size(), _value.size() - 1) + 1, _value.data());

                    if(imgui::selectable_input("##value", false, _value.data(), _value.size())) {
                        var.error = !var.from_string(_value.data());
                        var.modified = true;
                    }

                    if(ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(var.type_name.data());
                        ImGui::EndTooltip();
                    }

                    if(pop_color) {
                        ImGui::PopStyleColor();
                    }
                }

                ImGui::TableSetColumnIndex(2);

                {
                    if(var.modified) {
                        if(ImGui::Selectable(ICON_FA_UNDO)) {
                            y_always_assert(var.from_string(var.default_value), "Unable to reset value");
                            var.error = false;
                            var.modified = false;
                        }
                    }
                }


                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

}
