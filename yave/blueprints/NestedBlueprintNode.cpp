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
#include "BlueprintCompiler.h"

#include <y/serde3/archives.h>

#include <algorithm>

namespace yave {

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

    _output_names.clear();
    _input_pins.clear();
    _output_pins.clear();

    if(!_blueprint) {
        return;
    }

    const auto instance = _blueprint->create_instance();
    if(instance.is_error()) {
        return;
    }

    const core::Span<BlueprintParam> params_in = instance.unwrap().params_in();
    const core::Span<BlueprintParam> params_out = instance.unwrap().params_out();

    core::FixedArray<InputValue> values(params_in.size());
    for(usize i = 0; i != params_in.size(); ++i) {
        const BlueprintParam& param = params_in[i];

        const auto it = std::find_if(_input_values.begin(), _input_values.end(), [&](const InputValue& value) {
            return value.name == param.name && value.type == param.type->type_hash && value.value.size() == param.type->size;
        });

        const u8* data = it != _input_values.end() ? it->value.data() : static_cast<const u8*>(param.ptr);
        values[i] = InputValue{param.name, param.type->type_hash, core::Vector<u8>(core::Span<u8>(data, param.type->size))};
    }
    _input_values = std::move(values);

    _input_pins = core::FixedArray<BlueprintPin>(params_in.size());
    for(usize i = 0; i != params_in.size(); ++i) {
        _input_pins[i] = BlueprintPin{_input_values[i].name, params_in[i].type};
    }

    _output_names = core::FixedArray<core::String>(params_out.size());
    _output_pins = core::FixedArray<BlueprintPin>(params_out.size());
    for(usize i = 0; i != params_out.size(); ++i) {
        _output_names[i] = params_out[i].name;
        _output_pins[i] = BlueprintPin{_output_names[i], params_out[i].type};
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

void* NestedBlueprintNode::default_input(usize index) {
    y_debug_assert(index < _input_values.size());
    return _input_values[index].value.data();
}

void NestedBlueprintNode::compile(BlueprintCompiler& compiler) const {
    if(!_blueprint) {
        compiler.error(core::String("Nested blueprint is not loaded"));
        return;
    }

    core::FixedArray<BlueprintParam> inputs(_input_pins.size());
    for(usize i = 0; i != inputs.size(); ++i) {
        inputs[i] = BlueprintParam{_input_values[i].name, 0, _input_pins[i].type, compiler.input(i)};
    }

    auto outputs = compiler.compile_nested(*_blueprint, inputs);
    if(outputs.is_error()) {
        compiler.error(std::move(outputs.error()));
        return;
    }

    for(usize i = 0; i != _output_pins.size(); ++i) {
        const auto it = std::find_if(outputs.unwrap().begin(), outputs.unwrap().end(), [&](const BlueprintParam& param) {
            return param.name == _output_names[i] && param.type == _output_pins[i].type;
        });

        if(it == outputs.unwrap().end()) {
            compiler.error(core::String("Nested blueprint params don't match"));
            return;
        }

        compiler.bind_output(i, it->ptr);
    }
}

const AssetPtr<Blueprint>& NestedBlueprintNode::blueprint() const {
    return _blueprint;
}

}