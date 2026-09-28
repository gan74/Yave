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
#include "Blueprint.h"
#include "BlueprintData.h"

#include <y/core/ScratchPad.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <memory>

namespace yave {

static bool resolve_generic_link(BlueprintNode* src, usize src_pin, BlueprintNode* dst, usize dst_pin) {
    const BlueprintParamType* src_type = src->output_pins()[src_pin].type;
    const BlueprintParamType* dst_type = dst->input_pins()[dst_pin].type;
    if(!src_type && dst_type) {
        src->set_generic_type(dst_type);
        return true;
    }
    if(src_type && !dst_type) {
        dst->set_generic_type(src_type);
        return true;
    }
    return false;
}

Blueprint::Blueprint(BlueprintData data) : _nodes(std::move(data._nodes)) {
    y_profile();

    for(;;) {
        bool changed = false;
        for(const BlueprintLink& link : data._links) {
            y_debug_assert(link.src_node < _nodes.size());
            y_debug_assert(link.dst_node < _nodes.size());
            changed |= resolve_generic_link(_nodes[link.src_node].get(), link.src_pin, _nodes[link.dst_node].get(), link.dst_pin);
        }

        if(!changed) {
            break;
        }
    }

    for(const BlueprintLink& link : data._links) {
        const void* out = _nodes[link.src_node]->output_ptr(link.src_pin);
        _nodes[link.dst_node]->set_input(link.dst_pin, out);
    }
}

core::Span<std::unique_ptr<BlueprintNode>> Blueprint::all_nodes() const {
    return _nodes;
}

usize Blueprint::find_node_index(const BlueprintNode* node) const {
    const auto it = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == node; });
    return it == _nodes.end() ? usize(-1) : usize(it - _nodes.begin());
}

usize Blueprint::find_output_pin(const BlueprintNode& node, const void* ptr) {
    if(ptr) {
        const usize output_count = node.output_pins().size();
        for(usize i = 0; i != output_count; ++i) {
            if(node.output_ptr(i) == ptr) {
                return i;
            }
        }
    }
    return usize(-1);
}

void Blueprint::update_generic_types() {
    core::ScratchPad<bool> linked(_nodes.size(), false);
    for(usize k = 0; k != _nodes.size(); ++k) {
        const core::Span<BlueprintPin> inputs = _nodes[k]->input_pins();
        for(usize i = 0; i != inputs.size(); ++i) {
            if(const void* in = _nodes[k]->input(i)) {
                const auto [src, src_pin] = find_output(in);
                linked[k] |= inputs[i].is_generic;
                linked[find_node_index(src)] |= src->output_pins()[src_pin].is_generic;
            }
        }
    }

    for(usize i = 0; i != _nodes.size(); ++i) {
        if(!linked[i] && _nodes[i]->generic_type()) {
            _nodes[i]->set_generic_type(nullptr);
        }
    }
}

const BlueprintNode* Blueprint::add_node(std::unique_ptr<BlueprintNode> node) {
    BlueprintNode* bp_node = _nodes.emplace_back(std::move(node)).get();
    bp_node->reset_inputs();
    return bp_node;
}

void Blueprint::remove_node(const BlueprintNode* node) {
    const usize node_index = find_node_index(node);
    y_debug_assert(node_index < _nodes.size());

    for(usize k = node_index + 1; k != _nodes.size(); ++k) {
        BlueprintNode* dst = _nodes[k].get();
        const usize input_count = dst->input_pins().size();
        for(usize i = 0; i != input_count; ++i) {
            if(find_output_pin(*node, dst->input(i)) != usize(-1)) {
                dst->set_input(i, nullptr);
            }
        }
    }

    _nodes.erase(_nodes.begin() + node_index);

    update_generic_types();
}

void Blueprint::add_blueprint(Blueprint blueprint) {
    for(auto& node : blueprint._nodes) {
        _nodes.emplace_back(std::move(node));
    }
}

std::pair<const BlueprintNode*, usize> Blueprint::find_output(const void* ptr) const {
    if(ptr) {
        for(const auto& node : _nodes) {
            if(const usize pin = find_output_pin(*node, ptr); pin != usize(-1)) {
                return {node.get(), pin};
            }
        }
    }

    return {};
}

void Blueprint::clear_links() {
    for(auto& node : _nodes) {
        node->reset_inputs();
    }

    update_generic_types();
}

bool Blueprint::is_link_valid(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) const {
    y_profile();

    if(!src || !dst || src == dst) {
        return false;
    }

    const core::Span<BlueprintPin> src_outputs = src->output_pins();
    const core::Span<BlueprintPin> dst_inputs = dst->input_pins();
    if(src_pin >= src_outputs.size() || dst_pin >= dst_inputs.size()) {
        return false;
    }

    if(!are_blueprint_types_compatible(src_outputs[src_pin].type, dst_inputs[dst_pin].type)) {
        return false;
    }

    const usize src_index = find_node_index(src);
    if(src_index == usize(-1)) {
        return false;
    }

    const usize dst_index = find_node_index(dst);
    if(dst_index == usize(-1)) {
        return false;
    }

    if(src_index < dst_index) {
        return true;
    }

    core::ScratchPad<bool> reachable(src_index - dst_index + 1, false);
    reachable[0] = true;

    for(usize y = dst_index + 1; y <= src_index; ++y) {
        const BlueprintNode* node = _nodes[y].get();
        const usize input_count = node->input_pins().size();
        for(usize i = 0; i != input_count && !reachable[y - dst_index]; ++i) {
            if(const void* in = node->input(i)) {
                for(usize x = dst_index; x != y; ++x) {
                    if(reachable[x - dst_index] && find_output_pin(*_nodes[x], in) != usize(-1)) {
                        reachable[y - dst_index] = true;
                        break;
                    }
                }
            }
        }
    }

    return !reachable[src_index - dst_index];
}

void Blueprint::add_link(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) {
    y_profile();

    y_debug_assert(is_link_valid(src, src_pin, dst, dst_pin));

    const usize src_index = find_node_index(src);
    const usize dst_index = find_node_index(dst);

    resolve_generic_link(_nodes[src_index].get(), src_pin, _nodes[dst_index].get(), dst_pin);

    const void* out_ptr = _nodes[src_index]->output_ptr(src_pin);
    _nodes[dst_index]->set_input(dst_pin, out_ptr);

    update_generic_types();

    {
        usize index = src_index;
        while(index > dst_index) {
            y_debug_assert(index);
            std::swap(_nodes[index - 1], _nodes[index]);
            --index;
        }
    }
}

void Blueprint::remove_link(const BlueprintNode* dst, usize dst_pin) {
    const usize dst_index = find_node_index(dst);
    y_debug_assert(dst_index < _nodes.size());
    y_debug_assert(dst_pin < dst->input_pins().size());

    _nodes[dst_index]->set_input(dst_pin, nullptr);
    update_generic_types();
}

core::Result<void, BlueprintError> Blueprint::eval() noexcept {
    y_profile();

    for(usize i = 0; i != _nodes.size(); ++i) {
        try {
            _nodes[i]->eval();
        } catch(const std::exception& e) {
            return core::Err(BlueprintError{i, core::String(e.what())});
        }
    }

    return core::Ok();
}

}
