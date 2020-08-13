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

#include "UiComponent.h"

#include <yave/ecs/EntityWorld.h>

#include <imgui/yave_imgui.h>

// Remove
#include <yave/graphics/swapchain/FrameToken.h>

namespace editor {

void UiWidgetBase::paint(ecs::EntityWorld&, CmdBufferRecorder& rec) {
    paint_ui(rec, FrameToken{});
}

void UiWidgetBase::paint_ui(CmdBufferRecorder&, const FrameToken&) {
}


UiWidgetBase::UiWidgetBase(std::string_view title, u32) {
    set_title(title);
}

UiWidgetBase::~UiWidgetBase() {
}

void UiWidgetBase::set_title(std::string_view title) {
    Y_TODO(we need to use an other id to keep imguis setting stack)
    const core::String new_title = fmt("%##%", title, _entity_id.index());
    _title_with_id = std::move(new_title);
    _title_size = title.size();
}

void UiWidgetBase::close() {
    _visible = false;
}

void UiWidgetBase::show() {
    _visible = true;
}

const math::Vec2& UiWidgetBase::position() const {
    return _position;
}

const math::Vec2& UiWidgetBase::size() const {
    return _size;
}

math::Vec2ui UiWidgetBase::content_size() const {
    return _content_size;
}

bool UiWidgetBase::is_focussed() const {
    return _focussed;
}

bool UiWidgetBase::is_mouse_inside() const {
    return _mouse_inside;
}

void UiWidgetBase::update_attribs() {
    const math::Vec2 content_max = ImGui::GetWindowContentRegionMax();
    const math::Vec2 content_min = ImGui::GetWindowContentRegionMin();
    _position = ImGui::GetWindowPos();
    _size = ImGui::GetWindowSize();
    _docked = ImGui::IsWindowDocked();
    _focussed = ImGui::IsWindowFocused();
    _content_size = (content_max - content_min).max(math::Vec2(1.0f));

    const math::Vec2 relative_mouse_pos = math::Vec2(ImGui::GetIO().MousePos) - _position;
    const auto less = [](const math::Vec2& a, const math::Vec2& b) { return a.x() < b.x() && a.y() < b.y(); };
    _mouse_inside = less(relative_mouse_pos, content_max) && less(content_min, relative_mouse_pos);
}

ecs::EntityId UiComponent::create_widget(ecs::EntityWorld& world, std::unique_ptr<UiWidgetBase> wid) {
    const ecs::EntityId ent = world.create_entity();
    world.add_component<UiComponent>(ent, std::move(wid), ecs::EntityId());
    return ent;
}

}

