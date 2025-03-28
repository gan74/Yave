/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef EDITOR_UTILS_UI_H
#define EDITOR_UTILS_UI_H

#include <editor/editor.h>

#include <yave/ecs/ecs.h>
#include <yave/graphics/images/ImageView.h>
#include <yave/assets/AssetType.h>
#include <yave/utils/color.h>

#include <y/core/Vector.h>

#include <external/imgui/imgui.h>

namespace editor {

struct UiIcon {
    std::string_view icon;
    u32 color;
};

inline constexpr math::Vec4 to_y(ImVec4 v) { return {v.x, v.y, v.z, v.w}; }
inline constexpr math::Vec2 to_y(ImVec2 v) { return {v.x, v.y}; }

template<typename T>
inline constexpr ImVec4 to_im(math::Vec<4, T> v) { return {float(v.x()), float(v.y()), float(v.z()), float(v.w())}; }

template<typename T>
inline constexpr ImVec2 to_im(math::Vec<2, T> v) { return {float(v.x()), float(v.y())}; }

ImGuiKey to_imgui_key(Key k);
ImGuiMouseButton to_imgui_button(MouseButton b);

namespace imgui {

static constexpr const char* drag_drop_path_id = "YAVE_DRAG_DROP_PATH";
static constexpr const char* drag_drop_entity_id = "YAVE_DRAG_DROP_ENTITY";
static constexpr ImVec4 error_text_color = to_im(math::Vec4(1.0f, 0.3f, 0.3f, 1.0f));
static constexpr ImVec4 warning_text_color = to_im(math::Vec4(1.0f, 0.8f, 0.4f, 1.0f));

const u32 folder_icon_color = 0xFF62D6FF;    // Light yellow

u32 gizmo_color(usize axis);

bool should_open_context_menu();

math::Vec2 client_window_pos();
math::Vec2 from_client_pos(const math::Vec2& pos);

usize text_line_count(std::string_view text);

std::pair<math::Vec2, math::Vec2> compute_glyph_uv_size(const char* c);

void text_icon(const UiIcon& icon);

bool text_input(const char* name, core::String& str, ImGuiInputTextFlags flags = 0, const char* hint = "");
bool text_input_multiline(const char* name, core::String& str, const math::Vec2& size = {}, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None);
void text_read_only(const char* name, std::string_view str);

bool position_input(const char* str_id, math::Vec3& position);

bool asset_selector(AssetId id, AssetType type, std::string_view text, bool* clear = nullptr);
bool path_selector(const char* text, std::string_view path);
bool id_selector(ecs::EntityId& id, const EditorWorld& world, ecs::ComponentTypeIndex with_component = ecs::ComponentTypeIndex::invalid_index, bool* browse = nullptr);

bool search_bar(const char* text, char* buffer, usize buffer_size);

bool begin_suggestion_popup();
void end_suggestion_popup();

bool suggestion_item(const char* name, const char* shortcut = nullptr);

void table_begin_next_row(int col_index = 0);

bool selectable_icon(const UiIcon& icon, const char* str_id, bool selected, ImGuiSelectableFlags flags = 0);
bool icon_button(const UiIcon& icon, const char* str_id, bool selected, float icon_size, ImGuiSelectableFlags flags = 0);
bool icon_button(UiTexture icon, const char* str_id, bool selected, float icon_size, ImGuiSelectableFlags flags = 0);

bool selectable_input(const char* str_id, bool selected, char* buf, usize buf_size);


void indeterminate_progress_bar(const math::Vec2& size_arg = {}, float speed = 1.0f);

char spinner();
const char* ellipsis();

}
}

#endif // EDITOR_UTILS_UI_H

