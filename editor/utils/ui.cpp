/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <editor/context/EditorContext.h>
#include <editor/widgets/AssetSelector.h>
#include <editor/widgets/FileBrowser.h>

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>

#include <external/imgui/yave_imgui.h>

namespace editor {
namespace imgui {

bool should_open_context_menu() {
    return ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1);
}

bool asset_selector(ContextPtr ctx, AssetId id, AssetType type, std::string_view text, bool* clear) {
    static constexpr math::Vec2 button_size = math::Vec2(64.0f, 64.0f);

    ImGui::PushID(fmt_c_str("%_%_%", id.id(), uenum(type), text));
    ImGui::BeginGroup();

    const auto name = ctx->asset_store().name(id);
    const bool is_valid = name.is_ok();

    if(clear) {
        *clear = false;
    }

    bool ret = false;
    bool button = false;
    if(is_valid) {
        if(TextureView* image = ctx->thumbmail_cache().get_thumbmail(id).image) {
            button = true;
            ret = ImGui::ImageButton(image, button_size);
        }
    }

    if(!button) {
        ret = ImGui::Button(ICON_FA_FOLDER_OPEN, button_size + math::Vec2(ImGui::GetStyle().FramePadding) * 2.0f);
    }


    ImGui::SameLine();
    if(ImGui::GetContentRegionAvail().x > button_size.x() * 0.5f) {
        const auto clean_name = [=](auto&& n) { return ctx->asset_store().filesystem()->filename(n); };
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

bool path_selector(std::string_view text, const core::String& path) {
    static constexpr usize buffer_capacity = 1024;

    ImGui::PushID(text.data());
    ImGui::BeginGroup();

    ImGui::TextUnformatted(text.data());

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

}

}

