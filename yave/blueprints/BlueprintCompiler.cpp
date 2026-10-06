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

#include <cstring>

namespace yave {

BlueprintCompiler::BlueprintCompiler(BlueprintStorage& storage) : _storage(storage) {
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

void BlueprintCompiler::error(core::String error) {
    y_debug_assert(!error.is_empty());
    _error = std::move(error);
}

}
