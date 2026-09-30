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

#include <y/core/ScratchPad.h>

#include <y/serde3/archives.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace yave {

template<typename T>
static core::FixedArray<T*> find_params(const BlueprintInstance& instance) {
    const core::Span nodes = instance.all_nodes();
    core::ScratchVector<T*> params(nodes.size());
    for(const auto& node : instance.all_nodes()) {
        if(T* param = dynamic_cast<T*>(node.get())) {
            params.push_back(param);
        }
    }

    std::sort(params.begin(), params.end(), [](const T* a, const T* b) { 
        if(a->order() != b->order()) {
            return a->order() < b->order();
        }
        return a->name() < b->name();
    });

    return core::FixedArray<T*>(core::Span<T*>(params));
}

template<typename T>
static bool has_duplicated_names(const core::FixedArray<T*>& params) {
    for(usize i = 0; i != params.size(); ++i) {
        for(usize j = 0; j != i; ++j) {
            if(params[i]->name() == params[j]->name()) {
                return true;
            }
        }
    }
    return false;
}


NestedBlueprintNode::NestedBlueprintNode(core::String name, AssetPtr<Blueprint> blueprint) : BlueprintNode(std::move(name)) {
    set_blueprint(std::move(blueprint));
}

std::unique_ptr<BlueprintNode> NestedBlueprintNode::clone() const {
    auto node = std::make_unique<NestedBlueprintNode>();
    node->_name = _name;
    node->_input_values = core::FixedArray<InputValue>(core::Span<InputValue>(_input_values));
    node->set_blueprint(_blueprint);
    return node;
}

void NestedBlueprintNode::set_blueprint(AssetPtr<Blueprint> blueprint) {
    _blueprint = std::move(blueprint);

    _params_in.clear();
    _params_out.clear();
    _input_pins.clear();
    _output_pins.clear();
    _inputs.clear();

    if(!_blueprint) {
        _instance = core::Err(BlueprintError{0, core::String("Nested blueprint is not loaded")});
        return;
    }

    _instance = _blueprint->create_instance();
    if(_instance.is_error()) {
        return;
    }

    _params_in = find_params<ParamInBlueprintNode>(_instance.unwrap());
    _params_out = find_params<ParamOutBlueprintNode>(_instance.unwrap());

    _inputs = core::FixedArray<const void*>(_params_in.size());
    _input_pins = core::FixedArray<BlueprintPin>(_params_in.size());
    _output_pins = core::FixedArray<BlueprintPin>(_params_out.size());

    core::FixedArray<InputValue> values(_params_in.size());
    for(usize i = 0; i != _params_in.size(); ++i) {
        ParamInBlueprintNode* param = _params_in[i];
        const BlueprintParamType* type = param->generic_type();
        _input_pins[i] = BlueprintPin{param->name(), type};

        const auto it = std::find_if(_input_values.begin(), _input_values.end(), [&](const InputValue& value) {
            return value.name == param->name() && value.type == type->type_hash && value.value.size() == type->size;
        });

        const u8* data = it != _input_values.end() ? it->value.data() : static_cast<const u8*>(param->value());
        values[i] = InputValue{param->name(), type->type_hash, core::Vector<u8>(core::Span<u8>(data, type->size))};
    }
    _input_values = std::move(values);

    for(usize i = 0; i != _params_out.size(); ++i) {
        _output_pins[i] = BlueprintPin{_params_out[i]->name(), _params_out[i]->generic_type()};
    }

    if(has_duplicated_names(_params_in) || has_duplicated_names(_params_out)) {
        // params point into the instance, don't keep them around once it's gone
        _params_in.clear();
        _params_out.clear();
        _instance = core::Err(BlueprintError{0, core::String("Nested blueprint has several params with the same name")});
    }
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
    if(_instance.is_error()) {
        throw std::runtime_error(_instance.error().error.data());
    }

    for(usize i = 0; i != _params_in.size(); ++i) {
        const void* src = _inputs[i] ? _inputs[i] : _input_values[i].value.data();
        std::memcpy(_params_in[i]->value(), src, _input_pins[i].type->size);
    }

    for(const auto& node : _instance.unwrap().all_nodes()) {
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
    y_debug_assert(index < _input_values.size());
    return _input_values[index].value.data();
}

const void* NestedBlueprintNode::output_ptr(usize index) const {
    y_debug_assert(index < _params_out.size());
    return _params_out[index]->value();
}

const AssetPtr<Blueprint>& NestedBlueprintNode::blueprint() const {
    return _blueprint;
}

}
