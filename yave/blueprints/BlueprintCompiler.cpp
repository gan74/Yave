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

#include "BlueprintCompiler.h"
#include "Blueprint.h"

#include <algorithm>
#include <cstring>

namespace yave {

static void sort_params(core::Vector<BlueprintParam>& params) {
    std::sort(params.begin(), params.end(), [](const BlueprintParam& a, const BlueprintParam& b) {
        if(a.order != b.order) {
            return a.order < b.order;
        }
        return a.name < b.name;
    });
}

static bool has_duplicated_names(core::Span<BlueprintParam> params) {
    for(usize i = 1; i < params.size(); ++i) {
        for(usize j = 0; j != i; ++j) {
            if(params[i].name == params[j].name) {
                return true;
            }
        }
    }
    return false;
}




BlueprintCompiler::BlueprintCompiler(BlueprintStorage& storage, core::Span<BlueprintParam> inputs) : _storage(storage), _inputs(inputs) {
}

core::Result<void, BlueprintError> BlueprintCompiler::compile(core::Span<std::unique_ptr<BlueprintNode>> nodes, core::Span<BlueprintLink> links) {
    y_profile();

    _nodes = nodes;
    _links = links;
    _outputs = core::FixedArray<core::FixedArray<const void*>>(nodes.size());
    _instructions = core::FixedArray<core::Vector<BlueprintInstruction>>(nodes.size());

    for(_current = 0; _current != nodes.size(); ++_current) {
        _outputs[_current] = core::FixedArray<const void*>(nodes[_current]->output_pins().size());
        nodes[_current]->compile(*this);
        if(!_error.is_empty()) {
            return core::Err(BlueprintError{_current, std::move(_error)});
        }
    }

    sort_params(_params_in);
    sort_params(_params_out);

    if(has_duplicated_names(_params_in) || has_duplicated_names(_params_out)) {
        return core::Err(BlueprintError{0, core::String("Blueprint has several params with the same name")});
    }

    return core::Ok();
}

BlueprintNode* BlueprintCompiler::current_node() const {
    return _nodes[_current].get();
}

const void* BlueprintCompiler::input(usize index) {
    for(const BlueprintLink& link : _links) {
        if(link.dst_node == _current && link.dst_pin == index) {
            const void* ptr = _outputs[link.src_node][link.src_pin];
            y_debug_assert(ptr);
            return ptr;
        }
    }

    BlueprintNode* node = current_node();
    return alloc_copy(node->input_pins()[index].type, node->default_input(index));
}

void* BlueprintCompiler::output(usize index) {
    const BlueprintParamType* type = current_node()->output_pins()[index].type;
    void* ptr = alloc(type->size, type->alignment);
    bind_output(index, ptr);
    return ptr;
}

void BlueprintCompiler::bind_output(usize index, const void* ptr) {
    y_debug_assert(!_outputs[_current][index]);
    _outputs[_current][index] = ptr;
}

void* BlueprintCompiler::alloc(usize size, usize alignment) {
    return _storage.alloc(size, alignment);
}

void* BlueprintCompiler::alloc_copy(const BlueprintParamType* type, const void* value) {
    void* ptr = alloc(type->size, type->alignment);
    std::memcpy(ptr, value, type->size);
    return ptr;
}

void BlueprintCompiler::emit(std::function<void()> func) {
    _instructions[_current].emplace_back(BlueprintInstruction{u32(_current), std::move(func)});
}

void BlueprintCompiler::set_entry_point(ecs::TriggerTypeIndex type, void* payload, usize payload_size, void (*subscribe)(ecs::TriggerSubscriber&)) {
    _entry_points.emplace_back(BlueprintInstance::EntryPoint{u32(_current), type, payload, payload_size, subscribe, {}});
}

const void* BlueprintCompiler::param_in(const core::String& name, i32 order, const BlueprintParamType* type, const void* default_value) {
    const auto it = std::find_if(_inputs.begin(), _inputs.end(), [&](const BlueprintParam& param) {
        return param.name == name && param.type == type;
    });

    const void* ptr = it != _inputs.end() ? it->ptr : alloc_copy(type, default_value);
    _params_in.emplace_back(BlueprintParam{name, order, type, ptr});
    return ptr;
}

void BlueprintCompiler::param_out(const core::String& name, i32 order, const BlueprintParamType* type, const void* ptr) {
    _params_out.emplace_back(BlueprintParam{name, order, type, ptr});
}

core::Result<core::Vector<BlueprintParam>, core::String> BlueprintCompiler::compile_nested(const Blueprint& blueprint, core::Span<BlueprintParam> inputs) {
    BlueprintCompiler nested(_storage, inputs);
    if(auto res = blueprint.compile(nested); res.is_error()) {
        return core::Err(std::move(res.error().error));
    }

    if(!nested._entry_points.is_empty()) {
        return core::Err(core::String("Nested blueprints can't have entry points"));
    }

    for(core::Vector<BlueprintInstruction>& instructions : nested._instructions) {
        for(BlueprintInstruction& instruction : instructions) {
            emit(std::move(instruction.func));
        }
    }

    return core::Ok(std::move(nested._params_out));
}

void BlueprintCompiler::error(core::String error) {
    y_debug_assert(!error.is_empty());
    _error = std::move(error);
}

}
