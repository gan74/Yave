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

namespace detail {
BlueprintParamTypeIndex next_blueprint_param_type_index() {
    static std::atomic<std::underlying_type_t<BlueprintParamTypeIndex>> global_type_index = 0;
    return BlueprintParamTypeIndex(global_type_index++);
}
}


BlueprintNode::BlueprintNode(std::shared_ptr<SharedBlueprintNodeData> shared_data) : _shared_data(std::move(shared_data)) {
}

BlueprintNode::~BlueprintNode() {
}

std::string_view BlueprintNode::name() const {
    return _shared_data ? std::string_view(_shared_data->name) : std::string_view("Unnamed node");
}

std::string_view BlueprintNode::input_name(usize index) const {
    return _shared_data ? std::string_view(_shared_data->inputs[index].first) : std::string_view("Unnamed input");
}

std::string_view BlueprintNode::output_name(usize index) const {
    return _shared_data ? std::string_view(_shared_data->outputs[index].first) : std::string_view("Unnamed output");
}

std::string_view BlueprintNode::param_name(usize index) const {
    return _shared_data ? std::string_view(_shared_data->params[index].first) : std::string_view("Unnamed param");
}

BlueprintParamTypeIndex BlueprintNode::input_type(usize index) const {
    return _shared_data ? _shared_data->inputs[index].second : BlueprintParamTypeIndex::invalid_index;
}

BlueprintParamTypeIndex BlueprintNode::output_type(usize index) const {
    return _shared_data ? _shared_data->outputs[index].second : BlueprintParamTypeIndex::invalid_index;
}

BlueprintParamTypeIndex BlueprintNode::param_type(usize index) const {
    return _shared_data ? _shared_data->params[index].second : BlueprintParamTypeIndex::invalid_index;
}

void BlueprintNode::reset_inputs() {
    const usize c = input_count();
    for(usize i = 0; i != c; ++i) {
        set_input(i, nullptr);
    }
}

usize BlueprintNode::input_count() const {
    return 0;
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

usize BlueprintNode::output_count() const {
    return 0;
}

const void* BlueprintNode::output_ptr(usize) const {
    y_debug_assert(false);
    return nullptr;
}

usize BlueprintNode::param_count() const {
    return 0;
}

void* BlueprintNode::param_ptr(usize) {
    y_debug_assert(false);
    return nullptr;
}

}
