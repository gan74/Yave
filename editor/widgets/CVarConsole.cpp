
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

#include "CVarConsole.h"

#include <editor/utils/ui.h>
#include <editor/Settings.h>

#include <yave/utils/color.h>

#include <y/utils/name.h>
#include <y/utils/format.h>

#include <algorithm>
#include <utility>
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

    try {
        if constexpr(std::is_arithmetic_v<T>) {
            // Specialize for bool to allow "true"/"false"
            if constexpr(std::is_same_v<bool, T>) {
                static constexpr std::string_view true_value = "true";
                static constexpr std::string_view false_value = "false";
                auto eq_icase = [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); };
                if(std::equal(str.begin(), str.end(), true_value.begin(), true_value.end(), eq_icase)) {
                    t = true;
                    return true;
                }
                if(std::equal(str.begin(), str.end(), false_value.begin(), false_value.end(), eq_icase)) {
                    t = false;
                    return true;
                }
            }

            double val = 0.0;
            if(std::from_chars(str.data(), str.data() + str.size(), val).ec != std::errc()) {
                return false;
            }
            t = T(val);
            return true;
        } else if constexpr(std::is_constructible_v<std::string_view>) {
            t = T(str);
            return true;
        }
    } catch(...) {
    }
    return false;
}

static std::string_view trim_type_name(std::string_view name) {
    if(name.starts_with("class ")) {
        name = name.substr(6);
    } else if(name.starts_with("struct ")) {
        name = name.substr(7);
    }


    if(name.starts_with("y::core::")) {
        return name.substr(9);
    } else if(name.starts_with("y::math::")) {
        return name.substr(9);
    } else if(name.starts_with("yave::ecs::")) {
        return name.substr(11);
    } else if(name.starts_with("yave::")) {
        return name.substr(6);
    } else if(name.starts_with("editor::")) {
        return name.substr(8);
    } else if(name.starts_with("y::")) {
        return name.substr(3);
    }
    return name;
}

template<typename T, typename C, typename F>
static void collect_var(C& vars, const core::String& parent, std::string_view name, F&& getter, const T& default_value) {
    unused(vars, parent, name, getter);

    const core::String full_name = parent + "." + name;
    if constexpr(is_printable_v<T>) {
        vars.emplace_back(
            full_name,
            [=]{ return fmt_to_owned("{}", getter()); },
            [=](std::string_view str) { return try_parse(str, getter()); },
            trim_type_name(ct_type_name<T>()),
            fmt_to_owned("{}", default_value),
            false
        );
    } else if constexpr(is_iterable_v<T>) {
        if constexpr(is_printable_v<typename T::value_type>) {
            vars.emplace_back(
                full_name,
                [=]{ return fmt_to_owned("{}", getter()); },
                [=](std::string_view) { /* not supported */ return false; },
                trim_type_name(ct_type_name<T>()),
                fmt_to_owned("{}", default_value),
                false
            );
        }
    }
}

template<typename C, typename F, typename T>
static void explore(C& vars, const core::String& parent, std::string_view name, F&& getter, const T& default_value) {
    collect_var(vars, parent, name, getter, default_value);

    reflect::explore_members<T>([&, full = parent + "." + name](std::string_view name, auto member) mutable {
        using member_type = std::remove_cvref_t<decltype(std::declval<T>().*member)>;
        explore(vars, full, name, [getter, member]() -> member_type& {
            return getter().*member;
        }, default_value.*member);
    });
}




CVarConsole::CVarConsole() : Widget(ICON_FA_STREAM " CVars") {
    explore(_cvars, "", "settings", app_settings, Settings{});
}

void CVarConsole::on_gui() {
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(ICON_FA_FILTER " ").x);
    imgui::text_input(ICON_FA_FILTER "##filter", _search_pattern);
    const bool search_empty = _search_pattern.is_empty();

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

                const core::String value = var.to_string();
                const bool modified = var.default_value != value;

                {

                    bool pop_color = false;
                    if(var.error) {
                        ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
                        pop_color = true;
                    } else if(modified) {
                        ImGui::PushStyleColor(ImGuiCol_Text, math::Vec4(sRGB_to_linear(math::Vec3(0.3f, 0.3f, 1.0f)), 1.0f));
                        pop_color = true;
                    }

                    std::copy_n(value.data(), std::max(value.size(), _value.size() - 1) + 1, _value.data());

                    if(imgui::selectable_input("##value", false, _value.data(), _value.size())) {
                        var.error = !var.from_string(_value.data());
                    }

                    if(pop_color) {
                        ImGui::PopStyleColor();
                    }

                    /*if(ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s (default value: \"%s\")", var.type_name.data(), var.default_value.data());
                        ImGui::EndTooltip();
                    }*/
                }

                ImGui::TableSetColumnIndex(2);

                if(modified) {
                    if(ImGui::Selectable(ICON_FA_UNDO)) {
                        y_always_assert(var.from_string(var.default_value), "Unable to reset value");
                        var.error = false;
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
