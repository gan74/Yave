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

#include <y/core/ScratchPad.h>

#include <memory>

namespace yave {

const core::Span<std::unique_ptr<BlueprintNode>> Blueprint::all_nodes() const {
    return _nodes;
}

const BlueprintNode* Blueprint::add_node(std::unique_ptr<BlueprintNode> node) {
    BlueprintNode* bp_node = _nodes.emplace_back(std::move(node)).get();
    bp_node->reset_inputs();
    return bp_node;
}

void Blueprint::clear_links() {
    for(auto& node : _nodes) {
        node->reset_inputs();
    }
}

bool Blueprint::is_link_valid(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) const {
    y_profile();

    if(!src || !dst || src_pin >= src->output_count() || dst_pin >= dst->input_count()) {
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

    core::ScratchPad<bool> visited(_nodes.size(), false);
    core::ScratchVector<usize> stack(_nodes.size());
    stack.push_back(dst_index);
    while(!stack.is_empty()) {
        const usize index = stack.pop();
        if(index == src_index) {
            return false;
        }
        if(visited[index]) {
            continue;
        }
        visited[index] = true;

        const BlueprintNode* node = _nodes[index].get();
        for(usize y = 0; y != _nodes.size(); ++y) {
            if(!visited[y]) {
                for(usize i = 0; i != _nodes[y]->input_count(); ++i) {
                    if(const void* in = _nodes[y]->input(i)) {
                        for(usize o = 0; o != node->output_count(); ++o) {
                            if(node->output_ptr(o) == in) {
                                stack.push_back(y);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

void Blueprint::add_link(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) {
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

void Blueprint::eval() {
    y_profile();
    for(auto& node : _nodes) {
        node->eval();
    }
}

}
