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

#include "NestedBlueprintNode.h"

#include <y/serde3/archives.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace yave {

template<typename T>
static core::Vector<T*> find_params(const BlueprintInstance& instance) {
    core::Vector<T*> params;
    for(const auto& node : instance.all_nodes()) {
        if(T* param = dynamic_cast<T*>(node.get())) {
            params << param;
        }
    }
    std::stable_sort(params.begin(), params.end(), [](const T* a, const T* b) { return a->order() < b->order(); });
    return params;
}


NestedBlueprintNode::NestedBlueprintNode(core::String name, AssetPtr<Blueprint> blueprint) : BlueprintNode(std::move(name)), _blueprint(std::move(blueprint)), _result(core::Ok()) {
    y_debug_assert(_blueprint);

    if(auto res = _blueprint->create_instance()) {
        _instance = std::move(res.unwrap());
    } else {
        _result = core::Err(std::move(res.error()));
    }

    _params_in = find_params<ParamInBlueprintNode>(_instance);
    _params_out = find_params<ParamOutBlueprintNode>(_instance);

    for(const ParamInBlueprintNode* param : _params_in) {
        _input_pins << BlueprintPin{param->name(), param->generic_type()};
    }
    for(const ParamOutBlueprintNode* param : _params_out) {
        _output_pins << BlueprintPin{param->name(), param->generic_type()};
    }

    _inputs = core::FixedArray<const void*>(_params_in.size());
}

std::unique_ptr<BlueprintNode> NestedBlueprintNode::clone() const {
    auto node = std::make_unique<NestedBlueprintNode>(_name, _blueprint);

    for(usize i = 0; i != _params_in.size(); ++i) {
        std::memcpy(node->_params_in[i]->value(), _params_in[i]->value(), _input_pins[i].type->size);
    }
    return node;
}

std::string_view NestedBlueprintNode::node_type_name() const {
    return "NestedBlueprint";
}

core::Span<BlueprintPin> NestedBlueprintNode::input_pins() const {
    return _input_pins;
}

core::Span<BlueprintPin> NestedBlueprintNode::output_pins() const {
    return _output_pins;
}

void NestedBlueprintNode::eval() {
    if(_result.is_error()) {
        throw std::runtime_error(_result.error().error.data());
    }

    for(usize i = 0; i != _params_in.size(); ++i) {
        if(_inputs[i]) {
            std::memcpy(_params_in[i]->value(), _inputs[i], _input_pins[i].type->size);
        }
    }

    for(const auto& node : _instance.all_nodes()) {
        node->eval();
    }
}

void NestedBlueprintNode::set_input(usize index, const void* ptr) {
    y_debug_assert(index < _inputs.size());
    _inputs[index] = ptr;
}

const void* NestedBlueprintNode::input(usize index) const {
    y_debug_assert(index < _inputs.size());
    return _inputs[index];
}

void* NestedBlueprintNode::default_input(usize index) {
    y_debug_assert(index < _params_in.size());
    return _params_in[index]->value();
}

const void* NestedBlueprintNode::output_ptr(usize index) const {
    y_debug_assert(index < _params_out.size());
    return _params_out[index]->value();
}

const AssetPtr<Blueprint>& NestedBlueprintNode::blueprint() const {
    return _blueprint;
}

}
