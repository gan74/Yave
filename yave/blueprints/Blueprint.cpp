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

Blueprint::Blueprint(BlueprintData&& data) : _nodes(std::move(data._nodes)) {
    for(const BlueprintLink& link : data._links) {
        y_debug_assert(link.src_node < _nodes.size());
        y_debug_assert(link.dst_node < _nodes.size());
        const void* out = _nodes[link.src_node]->output_ptr(link.src_pin);
        _nodes[link.dst_node]->set_input(link.dst_pin, out);
    }
    data._links.make_empty();
}

const core::Span<std::unique_ptr<BlueprintNode>> Blueprint::all_nodes() const {
    return _nodes;
}

const BlueprintNode* Blueprint::add_node(std::unique_ptr<BlueprintNode> node) {
    BlueprintNode* bp_node = _nodes.emplace_back(std::move(node)).get();
    bp_node->reset_inputs();
    return bp_node;
}

std::pair<const BlueprintNode*, usize> Blueprint::find_output(const void* ptr) const {
    if(ptr) {
        for(const auto& node : _nodes) {
            const usize output_count = node->output_count();
            for(usize i = 0; i != output_count; ++i) {
                if(node->output_ptr(i) == ptr) {
                    return {node.get(), i};
                }
            }
        }
    }

    return {};
}

void Blueprint::clear_links() {
    for(auto& node : _nodes) {
        node->reset_inputs();
    }
}

bool Blueprint::is_link_valid(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) const {
    y_profile();

    if(!src || !dst || src == dst || src_pin >= src->output_count() || dst_pin >= dst->input_count()) {
        return false;
    }
    if(src->output_type(src_pin) != dst->input_type(dst_pin)) {
        return false;
    }

    const auto src_it = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == src; });
    const auto dst_it = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == dst; });
    if(src_it == _nodes.end() || dst_it == _nodes.end()) {
        return false;
    }

    const usize src_index = src_it - _nodes.begin();
    const usize dst_index = dst_it - _nodes.begin();

    if(src_index < dst_index) {
        return true;
    }

    core::ScratchPad<bool> visited(src_index - dst_index + 1, false);
    core::ScratchVector<usize> stack(src_index - dst_index + 1);
    stack.push_back(dst_index);
    while(!stack.is_empty()) {
        const usize index = stack.pop();
        if(index == src_index) {
            return false;
        }

        const usize visited_index = index - dst_index;
        if(visited[visited_index]) {
            continue;
        }
        visited[visited_index] = true;

        const BlueprintNode* node = _nodes[index].get();
        for(usize y = index + 1; y <= src_index; ++y) {
            if(visited[y - dst_index]) {
                continue;
            }
            for(usize i = 0; i != _nodes[y]->input_count(); ++i) {
                if(const void* in = _nodes[y]->input(i)) {
                    const usize output_count = node->output_count();
                    for(usize k = 0; k != output_count; ++k) {
                        if(node->output_ptr(k) == in) {
                            stack.push_back(y);
                            break;
                        }
                    }
                }
            }
        }
    }

    return true;
}

void Blueprint::add_link(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) {
    y_profile();

    y_debug_assert(is_link_valid(src, src_pin, dst, dst_pin));

    const auto src_it = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == src; });
    const auto dst_it = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == dst; });

    const usize src_index = src_it - _nodes.begin();
    const usize dst_index = dst_it - _nodes.begin();

    const void* out_ptr = _nodes[src_index]->output_ptr(src_pin);
    _nodes[dst_index]->set_input(dst_pin, out_ptr);

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
    const auto dst_it = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == dst; });
    y_debug_assert(dst_it != _nodes.end());
    y_debug_assert(dst_pin < dst->input_count());
    (*dst_it)->set_input(dst_pin, nullptr);
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
