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

#include "EditorComponent.h"

namespace editor {

EditorComponent::EditorComponent(core::String name) : _name(std::move(name)) {
}

const core::String& EditorComponent::name() const {
    return _name;
}

void EditorComponent::set_name(core::String name) {
    _name = std::move(name);
}

math::Vec3& EditorComponent::euler() {
    return _euler;
}

void EditorComponent::set_hidden_in_editor(bool hide) {
    _hide_in_editor = hide;
}

bool EditorComponent::is_hidden_in_editor() const {
    return _hide_in_editor;
}

void EditorComponent::set_parent_prefab(AssetId id) {
    _prefab = id;
}

AssetId EditorComponent::parent_prefab() const {
    return _prefab;
}

bool EditorComponent::is_prefab() const {
    return _prefab != AssetId::invalid_id();
}

}

