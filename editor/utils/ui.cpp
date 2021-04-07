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

#include "ui.h"
#include "assets.h"

#include <editor/ThumbmailRenderer.h>
#include <editor/widgets/AssetSelector.h>
#include <editor/widgets/FileBrowser.h>

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>

#include <external/imgui/imgui.h>
#include <external/imgui/imgui_internal.h>
#include <external/imgui/yave_imgui.h>

namespace editor {
namespace imgui {

bool should_open_context_menu() {
    return ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1);
}

math::Vec2 client_window_pos() {
    if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        return ImGui::GetWindowPos();
    }
    return math::Vec2(0.0f);
}

math::Vec2 from_client_pos(const math::Vec2& pos) {
    return client_window_pos() + pos;
}

bool asset_selector(AssetId id, AssetType type, std::string_view text, bool* clear) {
    static constexpr math::Vec2 button_size = math::Vec2(64.0f, 64.0f);

    ImGui::PushID(fmt_c_str("%_%_%", id.id(), uenum(type), text));
    ImGui::BeginGroup();

    const auto name = asset_store().name(id);
    const bool is_valid = name.is_ok();

    if(clear) {
        *clear = false;
    }

    bool ret = false;
    if(is_valid) {
        if(const TextureView* img = thumbmail_renderer().thumbmail(id)) {
            ret = ImGui::ImageButton(const_cast<TextureView*>(img), button_size);
        } else {
            ret = ImGui::Button(ICON_FA_QUESTION, button_size + math::Vec2(ImGui::GetStyle().FramePadding) * 2.0f);
        }
    } else {
        ret = ImGui::Button(ICON_FA_FOLDER_OPEN, button_size + math::Vec2(ImGui::GetStyle().FramePadding) * 2.0f);
    }


    ImGui::SameLine();
    if(ImGui::GetContentRegionAvail().x > button_size.x() * 0.5f) {
        const auto clean_name = [=](auto&& n) { return asset_store().filesystem()->filename(n); };
        const core::String clean = name.map(clean_name).unwrap_or(core::String());

        const bool is_empty = clean.is_empty();
        const char* item_name = is_empty ? "empty" : clean.data();

        ImGui::BeginGroup();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 2.0f);

        if(is_empty) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        }

        const bool combo = ImGui::BeginCombo("##combo", item_name);

        if(is_empty) {
            ImGui::PopStyleColor();
        }

        if(combo) {
            ImGui::Selectable(item_name, false, ImGuiSelectableFlags_Disabled);

            ImGui::Separator();
            if(ImGui::Selectable(ICON_FA_FOLDER_OPEN " Browse")) {
                ret = true;
            }

            if(!is_empty && clear) {
                ImGui::Separator();
                if(ImGui::Selectable(ICON_FA_TRASH " Clear")) {
                    *clear = true;
                }
            }

            ImGui::EndCombo();
        }
        ImGui::EndGroup();
    }

    ImGui::EndGroup();
    ImGui::PopID();
    return ret;
}

bool path_selector(const char* text, const core::String& path) {
    static constexpr usize buffer_capacity = 1024;

    ImGui::PushID(text);
    ImGui::BeginGroup();

    ImGui::TextUnformatted(text);

    std::array<char, buffer_capacity> buffer;
    {
        const bool end_with_slash = path.ends_with("/");
        const usize len = std::min(buffer.size() - (end_with_slash ? 1 : 2), path.size());
        std::copy_n(path.begin(), len, buffer.begin());
        buffer[len] = '/';
        buffer[len + !end_with_slash] = 0;
    }

    ImGui::InputText("", buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    const bool ret = ImGui::Button(ICON_FA_FOLDER_OPEN);

    ImGui::EndGroup();
    ImGui::PopID();
    return ret;
}

// from https://github.com/ocornut/imgui/issues/2668
void alternating_rows_background(float line_height, const math::Vec4& color) {
    const u32 im_color = ImGui::GetColorU32(color);

    auto* draw_list = ImGui::GetWindowDrawList();
    const auto& style = ImGui::GetStyle();

    if(line_height < 0) {
        line_height = ImGui::GetTextLineHeight();
    }

    line_height += style.ItemSpacing.y;

    float scroll_offset_h = ImGui::GetScrollX();
    float scroll_offset_v = ImGui::GetScrollY();
    float scrolled_out_lines = std::floor(scroll_offset_v / line_height);
    scroll_offset_v -= line_height * scrolled_out_lines;

    ImVec2 clip_rect_min(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
    ImVec2 clip_rect_max(clip_rect_min.x + ImGui::GetWindowWidth(), clip_rect_min.y + ImGui::GetWindowHeight());

    if(ImGui::GetScrollMaxX() > 0) {
        clip_rect_max.y -= style.ScrollbarSize;
    }

    draw_list->PushClipRect(clip_rect_min, clip_rect_max);


    const float y_min = clip_rect_min.y - scroll_offset_v + ImGui::GetCursorPosY();
    const float y_max = clip_rect_max.y - scroll_offset_v + line_height;
    const float x_min = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMin().x;
    const float x_max = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMax().x;

    bool is_odd = (static_cast<int>(scrolled_out_lines) % 2) == 0;
    for(float y = y_min; y < y_max; y += line_height, is_odd = !is_odd) {
        if(is_odd) {
            draw_list->AddRectFilled({ x_min, y - style.ItemSpacing.y }, { x_max, y + line_height }, im_color);
        }
    }

    draw_list->PopClipRect();
}




static int history_callback(ImGuiInputTextCallbackData* data) {
    int& selection = *static_cast<int*>(data->UserData);
    switch(data->EventKey) {
        case ImGuiKey_UpArrow:
            --selection;
        break;

        case ImGuiKey_DownArrow:
            ++selection;
        break;

        default:
        break;
    }
    return 0;
}

static struct SearchBarState {
        SearchBarState(ImGuiID i) : id(i) {}
        ImGuiID id = 0;
        int selection_index = -1;
        int item_count = 1;
        ImVec2 popup_pos;
        float popup_width = 0.0f;
        bool open_popup = false;
        bool enter_pressed = false;
        bool grab_focus = false;
    } search_bar_state(0);


bool search_bar(char* buffer, usize buffer_size) {
    ImGuiInputTextState* imgui_state = ImGui::GetInputTextState(ImGui::GetID(ICON_FA_SEARCH));

    if(imgui_state) {
        if(search_bar_state.id != imgui_state->ID) {
            search_bar_state = SearchBarState(imgui_state->ID);
        }
    }

    ImGui::SetNextItemWidth(-24.0f);

    search_bar_state.popup_pos = math::Vec2(ImGui::GetWindowPos()) + math::Vec2(ImGui::GetCursorPos());

    const bool grab_focus = imgui_state && search_bar_state.grab_focus;
    if(grab_focus) {
        ImGui::SetKeyboardFocusHere(0);
        search_bar_state.grab_focus = false;
        imgui_state->Stb.cursor = imgui_state->Stb.select_start = imgui_state->Stb.select_end = imgui_state->TextW.size();
    }

    const int flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    search_bar_state.enter_pressed = ImGui::InputText(ICON_FA_SEARCH, buffer, buffer_size, flags, history_callback, &search_bar_state.selection_index);

    const ImVec2 rect = ImGui::GetItemRectSize();
    search_bar_state.popup_pos.y += rect.y + 1.0f;
    search_bar_state.popup_width = rect.x;

    search_bar_state.open_popup = true;
    if(!buffer_size || !imgui_state) {
        search_bar_state.open_popup = false;
    }

    if(!buffer[0] && search_bar_state.selection_index < 0) {
        search_bar_state.open_popup = false;
    }

    return search_bar_state.enter_pressed && search_bar_state.selection_index < 0;
}

bool begin_suggestion_popup() {
    if(!search_bar_state.open_popup) {
        return false;
    }

    search_bar_state.selection_index = std::clamp(search_bar_state.selection_index, -1, search_bar_state.item_count - 1);
    search_bar_state.item_count = 0;

    const ImGuiWindowFlags popup_flags =
            ImGuiWindowFlags_NoFocusOnAppearing     |
            ImGuiWindowFlags_NoTitleBar             |
            ImGuiWindowFlags_AlwaysAutoResize       |
            ImGuiWindowFlags_NoResize               |
            ImGuiWindowFlags_NoMove                 |
            ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, math::Vec4(40.0f, 40.0f, 40.0f, 220.0f) / 255.0f);

    ImGui::SetNextWindowPos(search_bar_state.popup_pos);
    ImGui::SetNextWindowSize(ImVec2(search_bar_state.popup_width, 0.0f));
    const bool visible = ImGui::Begin("##suggestionpopup", nullptr, popup_flags);

    if(!visible) {
        end_suggestion_popup();
    }

    return visible;
}

void end_suggestion_popup() {
    if(search_bar_state.open_popup && !search_bar_state.item_count)  {
        ImGui::Selectable("no result", false, ImGuiSelectableFlags_Disabled);
    }

    ImGui::End();

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);
}

bool suggestion_item(const char* name) {
    y_debug_assert(search_bar_state.open_popup);

    bool selected = search_bar_state.item_count == search_bar_state.selection_index;
    bool activated = ImGui::Selectable(name, &selected);

    if(selected && search_bar_state.enter_pressed) {
        activated = true;
        search_bar_state.grab_focus = true;
    }

    if(selected || ImGui::IsItemHovered()) {
        search_bar_state.selection_index = search_bar_state.item_count;
    }
    if(activated) {
        search_bar_state.selection_index = -1;
    }

    ++search_bar_state.item_count;

    return activated;
}

}
}

