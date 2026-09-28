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

#include "BlueprintNode.h"

namespace yave {

BlueprintNode::BlueprintNode(core::String name) : _name(std::move(name)) {
}

BlueprintNode::~BlueprintNode() {
}

const core::String& BlueprintNode::name() const {
    return _name;
}

core::String& BlueprintNode::name() {
    return _name;
}

core::Span<BlueprintPin> BlueprintNode::input_pins() const {
    return {};
}

core::Span<BlueprintPin> BlueprintNode::output_pins() const {
    return {};
}

core::Span<BlueprintPin> BlueprintNode::param_pins() const {
    return {};
}

void BlueprintNode::set_generic_type(const BlueprintParamType* type) {
    unused(type);
    y_debug_assert(!type);
}

const BlueprintParamType* BlueprintNode::generic_type() const {
    return nullptr;
}

void BlueprintNode::reset_inputs() {
    const usize c = input_pins().size();
    for(usize i = 0; i != c; ++i) {
        set_input(i, nullptr);
    }
}

void BlueprintNode::set_input(usize, const void*) {
    y_debug_assert(false);
}

const void* BlueprintNode::input(usize) const {
    y_debug_assert(false);
    return nullptr;
}

void* BlueprintNode::default_input(usize) {
    y_debug_assert(false);
    return nullptr;
}

const void* BlueprintNode::output_ptr(usize) const {
    y_debug_assert(false);
    return nullptr;
}

void* BlueprintNode::param_ptr(usize) {
    y_debug_assert(false);
    return nullptr;
}

}
