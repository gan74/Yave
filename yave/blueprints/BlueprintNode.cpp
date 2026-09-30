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

bool BlueprintNode::has_generic_pin() const {
    for(const BlueprintPin& pin : input_pins()) {
        if(pin.is_generic) {
            return true;
        }
    }
    for(const BlueprintPin& pin : output_pins()) {
        if(pin.is_generic) {
            return true;
        }
    }
    return false;
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



static void set_generic_pin_type(BlueprintPin& pin, const BlueprintParamType* type, u64& value_type, core::FixedArray<u8>& value) {
    y_debug_assert(!pin.type || !type);
    pin.type = type;

    if(type && type->type_hash != value_type) {
        value_type = type->type_hash;
        value = core::FixedArray<u8>(type->size);
    }
}


ParamInBlueprintNode::ParamInBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
}

std::unique_ptr<BlueprintNode> ParamInBlueprintNode::clone() const {
    auto node = std::make_unique<ParamInBlueprintNode>(_name);
    node->_order = _order;
    node->_value_type = _value_type;
    node->_value = core::FixedArray<u8>(core::Span<u8>(_value));
    node->set_generic_type(generic_type());
    return node;
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

void ParamInBlueprintNode::set_generic_type(const BlueprintParamType* type) {
    set_generic_pin_type(_pins[0], type, _value_type, _value);
}

const BlueprintParamType* ParamInBlueprintNode::generic_type() const {
    return _pins[0].type;
}

void ParamInBlueprintNode::eval() {
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
    return _pins[0].type ? _value.data() : nullptr;
}

i32 ParamInBlueprintNode::order() const {
    return _order;
}

i32& ParamInBlueprintNode::order() {
    return _order;
}


ParamOutBlueprintNode::ParamOutBlueprintNode(core::String name) : BlueprintNode(std::move(name)) {
}

std::unique_ptr<BlueprintNode> ParamOutBlueprintNode::clone() const {
    auto node = std::make_unique<ParamOutBlueprintNode>(_name);
    node->_order = _order;
    node->_value_type = _value_type;
    node->_value = core::FixedArray<u8>(core::Span<u8>(_value));
    node->set_generic_type(generic_type());
    return node;
}

std::string_view ParamOutBlueprintNode::node_type_name() const {
    return "ParamOut";
}

core::Span<BlueprintPin> ParamOutBlueprintNode::input_pins() const {
    return _pin;
}

core::Span<BlueprintPin> ParamOutBlueprintNode::param_pins() const {
    return _order_pin;
}

void ParamOutBlueprintNode::set_generic_type(const BlueprintParamType* type) {
    set_generic_pin_type(_pin, type, _value_type, _value);
}

const BlueprintParamType* ParamOutBlueprintNode::generic_type() const {
    return _pin.type;
}

void ParamOutBlueprintNode::eval() {
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
    return _pin.type ? _value.data() : nullptr;
}

const void* ParamOutBlueprintNode::value() const {
    if(_input) {
        return _input;
    }
    return _pin.type ? _value.data() : nullptr;
}

void* ParamOutBlueprintNode::param_ptr(usize index) {
    unused(index);
    y_debug_assert(index == 0);
    return &_order;
}

i32 ParamOutBlueprintNode::order() const {
    return _order;
}

i32& ParamOutBlueprintNode::order() {
    return _order;
}

}
