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
#include "EntityPrefab.h"

namespace yave {
namespace ecs {

EntityPrefab::EntityPrefab(EntityId id) : _id(id) {
}

bool EntityPrefab::is_empty() const {
    return !_id.is_valid();
}

void EntityPrefab::add_box(std::unique_ptr<ComponentBoxBase> box) {
    _components.emplace_back(std::move(box));
}

void EntityPrefab::add_child(std::unique_ptr<EntityPrefab> prefab) {
    _children.emplace_back(std::move(prefab));
}

void EntityPrefab::add_child(AssetPtr<EntityPrefab> prefab) {
    _asset_children.emplace_back(std::move(prefab));
}

core::Span<std::unique_ptr<ComponentBoxBase>> EntityPrefab::components() const {
    return _components;
}

core::Span<std::unique_ptr<EntityPrefab>> EntityPrefab::children() const {
    return _children;
}

core::Span<AssetPtr<EntityPrefab>> EntityPrefab::asset_children() const {
    return _asset_children;
}

EntityId EntityPrefab::original_id() const {
    return _id;
}

}
}

