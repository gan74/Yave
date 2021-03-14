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
#ifndef EDITOR_UTILS_UI_H
#define EDITOR_UTILS_UI_H

#include <editor/editor.h>

#include <yave/assets/AssetId.h>

namespace editor {
namespace imgui {

bool should_open_context_menu();

math::Vec2 client_window_pos();
math::Vec2 from_client_pos(const math::Vec2& pos);


bool asset_selector(AssetId id, AssetType type, std::string_view text, bool* clear = nullptr);
bool path_selector(const char* text, const core::String& path);

void alternating_rows_background(float line_height = -1.0f, const math::Vec4& color = math::Vec4(20.0f, 20.0f, 20.0f, 48.0f) / 255.0f);


bool search_bar(char* buffer, usize buffer_size);

bool begin_suggestion_popup();
void end_suggestion_popup();

bool suggestion_item(const char* name);

}
}

#endif // EDITOR_UTILS_UI_H

