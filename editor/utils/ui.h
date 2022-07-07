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
#ifndef EDITOR_UTILS_UI_H
#define EDITOR_UTILS_UI_H

#include <editor/editor.h>

#include <yave/assets/AssetId.h>


namespace editor {
namespace imgui {

static constexpr const char* drag_drop_path_id = "YAVE_DRAG_DROP_PATH";
static constexpr math::Vec4 error_text_color = math::Vec4(1.0f, 0.3f, 0.3f, 1.0f);
static constexpr math::Vec4 warning_text_color = math::Vec4(1.0f, 0.8f, 0.4f, 1.0f);

u32 gizmo_color(usize axis);

bool should_open_context_menu();

math::Vec2 client_window_pos();
math::Vec2 from_client_pos(const math::Vec2& pos);

bool position_input(const char* str_id, math::Vec3& position);

bool asset_selector(AssetId id, AssetType type, std::string_view text, bool* clear = nullptr);
bool path_selector(const char* text, const core::String& path);

void alternating_rows_background(float line_height = -1.0f);


bool search_bar(const char* text, char* buffer, usize buffer_size);

bool begin_suggestion_popup();
void end_suggestion_popup();

bool suggestion_item(const char* name, const char* shortcut = nullptr);

void table_begin_next_row(int row_index = 0);


bool selectable_input(const char* str_id, bool selected, char* buf, usize buf_size);


void indeterminate_progress_bar(const math::Vec2& size_arg = {}, float speed = 1.0f);

char spinner();
const char* ellipsis();

}
}

#endif // EDITOR_UTILS_UI_H

