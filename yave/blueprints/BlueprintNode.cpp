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

#include <y/serde3/archives.h>

#include <algorithm>
#include <memory>

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

bool BlueprintNode::is_entry_point() const {
    return false;
}

bool BlueprintNode::has_generic_pin() const {
    const auto is_generic = [](const BlueprintPin& pin) { return pin.is_generic; };
    const core::Span<BlueprintPin> inputs = input_pins();
    const core::Span<BlueprintPin> outputs = output_pins();
    return std::any_of(inputs.begin(), inputs.end(), is_generic) || std::any_of(outputs.begin(), outputs.end(), is_generic);
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



ParamBlueprintNodeBase::ParamBlueprintNodeBase(std::string_view pin_name, core::String name) :
        BlueprintNode(std::move(name)),
        _pins({{{pin_name, nullptr, true}, {"order", blueprint_param_type<i32>()}}}) {
}

void ParamBlueprintNodeBase::set_generic_type(const BlueprintParamType* type) {
    y_debug_assert(!_pins[0].type || !type);
    _pins[0].type = type;

    if(type && type->type_hash != _value_type) {
        _value_type = type->type_hash;
        _value = core::FixedArray<u8>(type->size);
    }
}

const BlueprintParamType* ParamBlueprintNodeBase::generic_type() const {
    return _pins[0].type;
}

void ParamBlueprintNodeBase::eval() {
}

i32 ParamBlueprintNodeBase::order() const {
    return _order;
}

i32& ParamBlueprintNodeBase::order() {
    return _order;
}

void* ParamBlueprintNodeBase::default_value() {
    return _pins[0].type ? _value.data() : nullptr;
}


ParamInBlueprintNode::ParamInBlueprintNode(core::String name) : ParamBlueprintNodeBase("out", std::move(name)) {
}

std::unique_ptr<BlueprintNode> ParamInBlueprintNode::clone() const {
    return clone_as<ParamInBlueprintNode>();
}

std::string_view ParamInBlueprintNode::node_type_name() const {
    return "ParamIn";
}

core::Span<BlueprintPin> ParamInBlueprintNode::output_pins() const {
    return core::Span<BlueprintPin>(_pins.data(), 1);
}

core::Span<BlueprintPin> ParamInBlueprintNode::param_pins() const {
    return _pins;
}

const void* ParamInBlueprintNode::output_ptr(usize index) const {
    unused(index);
    y_debug_assert(index == 0);
    return _pins[0].type ? _value.data() : nullptr;
}

void* ParamInBlueprintNode::param_ptr(usize index) {
    y_debug_assert(index < _pins.size());
    return index ? static_cast<void*>(&_order) : value();
}

void* ParamInBlueprintNode::value() {
    return default_value();
}


ParamOutBlueprintNode::ParamOutBlueprintNode(core::String name) : ParamBlueprintNodeBase("in", std::move(name)) {
}

std::unique_ptr<BlueprintNode> ParamOutBlueprintNode::clone() const {
    return clone_as<ParamOutBlueprintNode>();
}

std::string_view ParamOutBlueprintNode::node_type_name() const {
    return "ParamOut";
}

core::Span<BlueprintPin> ParamOutBlueprintNode::input_pins() const {
    return core::Span<BlueprintPin>(_pins.data(), 1);
}

core::Span<BlueprintPin> ParamOutBlueprintNode::param_pins() const {
    return core::Span<BlueprintPin>(_pins.data() + 1, 1);
}

void ParamOutBlueprintNode::set_input(usize index, const void* ptr) {
    unused(index);
    y_debug_assert(index == 0);
    _input = ptr;
}

const void* ParamOutBlueprintNode::input(usize index) const {
    unused(index);
    y_debug_assert(index == 0);
    return _input;
}

void* ParamOutBlueprintNode::default_input(usize index) {
    unused(index);
    y_debug_assert(index == 0);
    return default_value();
}

const void* ParamOutBlueprintNode::value() const {
    if(_input) {
        return _input;
    }
    return _pins[0].type ? _value.data() : nullptr;
}

void* ParamOutBlueprintNode::param_ptr(usize index) {
    unused(index);
    y_debug_assert(index == 0);
    return &_order;
}

}
