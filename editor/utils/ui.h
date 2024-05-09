/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include <external/imgui/yave_imgui.h>

namespace editor {

class UiTexture {
    struct Data : NonMovable {
        Image<ImageUsage::TextureBit | ImageUsage::TransferDstBit | ImageUsage::ColorBit> texture;
        TextureView view;

        Data() = default;

        Data(ImageFormat format, const math::Vec2ui& size) : texture(format, size, MemoryAllocFlags::NoDedicatedAllocBit), view(texture) {
        }

        Data(TextureView v) : view(v) {
        }
    };

    static core::Vector<std::unique_ptr<Data>> _all_textures;

    public:
        static void clear_all() {
            _all_textures.clear();
        }

        static const TextureView* view(ImTextureID id) {
            return id ? &_all_textures[id - 1]->view : nullptr;
        }

        UiTexture() = default;

        UiTexture(ImageFormat format, const math::Vec2ui& size)  {
            _all_textures.emplace_back(std::make_unique<Data>(format, size));
            _id = ImTextureID(_all_textures.size());
        }

        UiTexture(TextureView view) {
            y_debug_assert(!view.is_null());
            _all_textures.emplace_back(std::make_unique<Data>(view));
            _id = ImTextureID(_all_textures.size());
        }

        const auto& texture() {
            y_debug_assert(_id);
            y_debug_assert(!_all_textures[_id - 1]->texture.is_null());
            return _all_textures[_id - 1]->texture;
        }

        operator bool() const {
            return _id;
        }

        ImTextureID to_imgui() const {
            y_debug_assert(_id);
            return _id;
        }

    private:
        ImTextureID _id = {};
};

struct UiIcon {
    std::string_view icon;
    u32 color;
};

ImGuiKey to_imgui_key(Key k);
ImGuiMouseButton to_imgui_button(MouseButton b);

namespace imgui {

static constexpr const char* drag_drop_path_id = "YAVE_DRAG_DROP_PATH";
static constexpr const char* drag_drop_entity_id = "YAVE_DRAG_DROP_ENTITY";
static const math::Vec4 error_text_color = math::Vec4(sRGB_to_linear(math::Vec3(1.0f, 0.3f, 0.3f)), 1.0f);
static const math::Vec4 warning_text_color = math::Vec4(sRGB_to_linear(math::Vec3(1.0f, 0.8f, 0.4f)), 1.0f);

const u32 folder_icon_color = 0xFF62D6FF;    // Light yellow

u32 gizmo_color(usize axis);

bool should_open_context_menu();

math::Vec2 client_window_pos();
math::Vec2 from_client_pos(const math::Vec2& pos);

usize text_line_count(std::string_view text);

std::pair<math::Vec2, math::Vec2> compute_glyph_uv_size(const char* c);

void text_icon(const UiIcon& icon);

bool text_input(const char* name, core::String& str, ImGuiInputTextFlags flags = 0, const char* hint = "");
bool text_input_multiline(const char* name, core::String& str);
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
bool icon_button(const UiIcon& icon, const char* str_id, bool selected, float icon_size);
bool icon_button(const UiTexture& icon, const char* str_id, bool selected, float icon_size);

bool selectable_input(const char* str_id, bool selected, char* buf, usize buf_size);


void indeterminate_progress_bar(const math::Vec2& size_arg = {}, float speed = 1.0f);

char spinner();
const char* ellipsis();

}
}

#endif // EDITOR_UTILS_UI_H

