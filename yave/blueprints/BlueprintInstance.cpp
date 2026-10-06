/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#include "BlueprintInstance.h"

#include <y/utils/log.h>

#include <algorithm>

namespace yave {

core::Span<std::unique_ptr<BlueprintNode>> BlueprintInstance::all_nodes() const {
    return _nodes;
}

core::Span<BlueprintInstance::EntryPoint> BlueprintInstance::entry_points() const {
    return _entry_points;
}

core::Result<void, BlueprintError> BlueprintInstance::eval(const EntryPoint& entry_point) noexcept {
    y_profile();

    for(BlueprintNode* node : entry_point.nodes) {
        try {
            node->eval();
        } catch(const std::exception& e) {
            const usize index = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == node; }) - _nodes.begin();
            return core::Err(BlueprintError{index, core::String(e.what())});
        }
    }

    return core::Ok();
}

}
