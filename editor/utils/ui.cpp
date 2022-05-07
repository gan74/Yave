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

#include <charconv>
#include <cinttypes>
#include <cstdio>
#include <ctime>

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


bool position_input(const char* str_id, math::Vec3& position) {
    ImGui::PushID(str_id);
    y_defer(ImGui::PopID());

    const float width = ImGui::CalcItemWidth();
    bool edited = false;

    ImGui::Dummy(ImVec2());

    const char* text[] = {"X", "Y", "Z"};
    const char* input_name[] = {"##x", "##y", "##z"};

    for(usize i = 0; i != 3; ++i) {
        ImGui::SameLine();
        ImGui::BeginGroup();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());

        ImGui::SetNextItemWidth(width / 3.0f);
        //edited |= ImGui::InputFloat(input_name[i], &position[i], 0.0f, 0.0f, "%.2f");
        edited |= ImGui::DragFloat(input_name[i], &position[i], 1.0f, 0.0f, 0.0f, "%.2f");

        ImGui::SameLine();

        math::Vec4 color = math::Vec4(0.0f, 0.0f, 0.0f, 0.5f);
        color[i] = 1.0f;

        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);

        ImGui::Button(text[i]);

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndGroup();
    }

    return edited;
}

bool asset_selector(AssetId id, AssetType type, std::string_view text, bool* clear) {
    static constexpr math::Vec2 button_size = math::Vec2(64.0f, 64.0f);
    const math::Vec2 padded_button_size =  button_size + math::Vec2(ImGui::GetStyle().FramePadding) * 2.0f;

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
            ret = ImGui::Button(ICON_FA_HOURGLASS_HALF, padded_button_size);
        }
    } else {
        if(id == AssetId::invalid_id()) {
            ret = ImGui::Button(ICON_FA_FOLDER_OPEN, padded_button_size);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, error_text_color);
            ret = ImGui::Button(ICON_FA_EXCLAMATION_CIRCLE, padded_button_size);
            ImGui::PopStyleColor();
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Asset with id %016" PRIx64 " could not be loaded", id.id());
            }
        }
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


static struct SearchBarState {
    SearchBarState(ImGuiID i) : id(i) {}
    ImGuiID id = 0;
    int selection_index = -1;
    int item_count = 1;
    ImVec2 popup_pos;
    float popup_width = 0.0f;
    bool open_popup = false;
    bool keep_open = false;
    bool activated = false;
    bool was_activated = false;
    bool focussed = false;
} search_bar_state(0);



bool search_bar(const char* text, char* buffer, usize buffer_size) {
    // Position
    {
        const float text_width = ImGui::CalcTextSize(text, nullptr, true).x + 8.0f;
        ImGui::SetNextItemWidth(-text_width);
        search_bar_state.popup_pos = math::Vec2(ImGui::GetWindowPos()) + math::Vec2(ImGui::GetCursorPos());
    }

    ImGuiInputTextState* imgui_state = ImGui::GetInputTextState(ImGui::GetID(text));
    search_bar_state.focussed = imgui_state;


    if(!search_bar_state.focussed) {
        ImGui::InputText(text, buffer, buffer_size);
        return false;
    }

    if(search_bar_state.id != imgui_state->ID) {
        search_bar_state = SearchBarState(imgui_state->ID);
    }


    const bool was_activated_last_frame = search_bar_state.was_activated;
    search_bar_state.was_activated = search_bar_state.activated;
    if(search_bar_state.was_activated) {
        ImGui::SetKeyboardFocusHere();
    }

    // Input handling
    {
        auto call_back = [](ImGuiInputTextCallbackData* data) {
            SearchBarState& state = *static_cast<SearchBarState*>(data->UserData);
            if(data->EventKey == ImGuiKey_UpArrow) {
                --state.selection_index;
            } else if(data->EventKey == ImGuiKey_DownArrow) {
                ++state.selection_index;
            }
            return 0;
        };

        search_bar_state.activated = ImGui::InputText(text, buffer, buffer_size, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory, call_back, &search_bar_state);
    }

    if(was_activated_last_frame) {
        imgui_state->SelectAll();
    }


    // Popup position
    {
        const ImVec2 rect = ImGui::GetItemRectSize();
        search_bar_state.popup_pos.y += rect.y + 1.0f;
        search_bar_state.popup_width = rect.x;
    }

    // Popup open status
    {
        search_bar_state.open_popup = search_bar_state.keep_open || ImGui::GetFocusID() == search_bar_state.id;
        if(!buffer_size || !imgui_state) {
            search_bar_state.open_popup = false;
        }

        if(!buffer[0] && search_bar_state.selection_index < 0) {
            search_bar_state.open_popup = false;
        }

        search_bar_state.keep_open = false;
    }

    return search_bar_state.activated && search_bar_state.selection_index < 0;
}

bool begin_suggestion_popup() {
    if(!search_bar_state.focussed || !search_bar_state.open_popup) {
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
    y_debug_assert(search_bar_state.focussed);

    if(search_bar_state.open_popup && !search_bar_state.item_count)  {
        ImGui::Selectable("no result", false, ImGuiSelectableFlags_Disabled);
    }

    ImGui::End();

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(1);
}

bool suggestion_item(const char* name, const char* shortcut) {
    y_debug_assert(search_bar_state.focussed);
    y_debug_assert(search_bar_state.open_popup);

    bool selected = search_bar_state.item_count == search_bar_state.selection_index;
    bool activated = ImGui::Selectable(name, &selected) || ImGui::IsItemActivated();
    const bool hovered = ImGui::IsItemHovered();

    if(shortcut) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", shortcut);
    }

    search_bar_state.keep_open |= hovered;

    if(selected && search_bar_state.activated) {
        activated = true;
    }

    if(selected || hovered) {
        search_bar_state.selection_index = search_bar_state.item_count;
    }

    if(activated) {
        search_bar_state.selection_index = -1;
        search_bar_state.activated = true;
    }

    ++search_bar_state.item_count;
    return activated;
}


void table_begin_next_row(int row_index) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(row_index);
}



// https://github.com/ocornut/imgui/issues/2718
bool selectable_input(const char* str_id, bool selected, char* buf, usize buf_size) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImVec2 pos_before = window->DC.CursorPos;

    ImGui::PushID(str_id);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));

    const bool clicked = ImGui::Selectable("##Selectable", selected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowItemOverlap);

    ImGui::PopStyleVar();


    ImGuiID id = window->GetID("##Input");
    const bool temp_input_is_active = ImGui::TempInputIsActive(id);
    const bool temp_input_start = clicked ? ImGui::IsMouseDoubleClicked(0) : false;

    if(temp_input_start) {
        ImGui::SetActiveID(id, window);
    }

    bool ret = false;
    if(temp_input_is_active || temp_input_start) {
        ImVec2 pos_after = window->DC.CursorPos;
        window->DC.CursorPos = pos_before;
        ret = ImGui::TempInputText(g.LastItemData.Rect, id, "##Input", buf, int(buf_size), ImGuiInputTextFlags_EnterReturnsTrue);
        window->DC.CursorPos = pos_after;
    } else {
        window->DrawList->AddText(pos_before, ImGui::GetColorU32(ImGuiCol_Text), buf);
    }

    ImGui::PopID();
    return ret;
}

char spinner() {
    // https://github.com/ocornut/imgui/issues/1901#issuecomment-400563921
    return "|/-\\"[usize(ImGui::GetTime() / 0.05f) & 3];
}

const char* ellipsis() {
    static const char* states[3] = { ".", "..", "..." };
    return states[usize(ImGui::GetTime() / 0.5f) % 3];
}

}
}

