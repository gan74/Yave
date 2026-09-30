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
#include "NestedBlueprintNode.h"

#include <y/core/ScratchPad.h>
#include <y/utils/log.h>

#include <algorithm>

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




core::Span<std::unique_ptr<BlueprintNode>> Blueprint::all_nodes() const {
    return _nodes;
}

core::Span<BlueprintLink> Blueprint::links() const {
    return _links;
}

core::Span<AssetPtr<Blueprint>> Blueprint::nested_blueprints() const {
    return _nested;
}

bool Blueprint::contains_nested(AssetId id) const {
    for(const AssetPtr<Blueprint>& nested : _nested) {
        if(nested.id() == id || (nested && nested->contains_nested(id))) {
            return true;
        }
    }
    return false;
}

usize Blueprint::find_node_index(const BlueprintNode* node) const {
    const auto it = std::find_if(_nodes.begin(), _nodes.end(), [=](const auto& n) { return n.get() == node; });
    return it == _nodes.end() ? usize(-1) : usize(it - _nodes.begin());
}

const BlueprintLink* Blueprint::find_link(usize dst_node, usize dst_pin) const {
    for(const BlueprintLink& link : _links) {
        if(link.dst_node == dst_node && link.dst_pin == dst_pin) {
            return &link;
        }
    }
    return nullptr;
}

void Blueprint::register_nested(const BlueprintNode* node) {
    if(const auto* nested = dynamic_cast<const NestedBlueprintNode*>(node)) {
        const AssetPtr<Blueprint>& blueprint = nested->blueprint();
        if(blueprint.id() != AssetId::invalid_id()) {
            if(const auto it = std::find(_nested.begin(), _nested.end(), blueprint); it == _nested.end()) {
                _nested.emplace_back(blueprint);
            } else if(!it->is_loaded()) {
                *it = blueprint;
            }
        }

    }
}

const BlueprintNode* Blueprint::add_node(std::unique_ptr<BlueprintNode> node) {
    register_nested(node.get());
    return _nodes.emplace_back(std::move(node)).get();
}

void Blueprint::remove_node(const BlueprintNode* node) {
    y_profile();

    const usize node_index = find_node_index(node);
    y_debug_assert(node_index < _nodes.size());

    core::Vector<BlueprintLink> links;
    for(BlueprintLink link : _links) {
        if(link.src_node == node_index || link.dst_node == node_index) {
            continue;
        }
        link.src_node -= link.src_node > node_index;
        link.dst_node -= link.dst_node > node_index;
        links << link;
    }
    _links = std::move(links);

    _nodes.erase(_nodes.begin() + node_index);

    _nested.make_empty();
    for(const auto& n : _nodes) {
        register_nested(n.get());
    }

    resolve_generic_types();
}

void Blueprint::add_blueprint(Blueprint data) {
    y_profile();

    const u32 offset = u32(_nodes.size());

    for(auto& node : data._nodes) {
        register_nested(node.get());
        _nodes.emplace_back(std::move(node));
    }

    for(BlueprintLink link : data._links) {
        link.src_node += offset;
        link.dst_node += offset;
        _links << link;
    }

    resolve_generic_types();
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

    const usize src_index = find_node_index(src);
    if(src_index == usize(-1)) {
        return false;
    }

    const usize dst_index = find_node_index(dst);
    if(dst_index == usize(-1)) {
        return false;
    }

    const BlueprintParamType* src_type = src_outputs[src_pin].type;
    const BlueprintParamType* dst_type = dst_inputs[dst_pin].type;
    if(src_type && dst_type && src_type != dst_type) {
        return false;
    }

    if(src_index < dst_index) {
        return true;
    }

    core::ScratchPad<bool> reachable(src_index - dst_index + 1, false);
    reachable[0] = true;

    for(usize y = dst_index + 1; y <= src_index; ++y) {
        for(const BlueprintLink& link : _links) {
            if(link.dst_node == y && link.src_node >= dst_index && link.src_node < y && reachable[link.src_node - dst_index]) {
                reachable[y - dst_index] = true;
                break;
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

    for(usize i = 0; i != _links.size(); ++i) {
        if(_links[i].dst_node == dst_index && _links[i].dst_pin == dst_pin) {
            _links.erase_unordered(_links.begin() + i);
            break;
        }
    }

    _links << BlueprintLink{u32(src_index), u32(src_pin), u32(dst_index), u32(dst_pin)};

    if(src_index > dst_index) {
        sort_nodes();
    }

    resolve_generic_types();
}

void Blueprint::sort_nodes() {
    y_profile();

    const usize node_count = _nodes.size();

    // Kahn's algorithm
    core::FixedArray<u32> first_edge(node_count + 1);
    core::FixedArray<u32> in_degrees(node_count);
    for(const BlueprintLink& link : _links) {
        ++first_edge[link.src_node + 1];
        ++in_degrees[link.dst_node];
    }
    for(usize i = 0; i != node_count; ++i) {
        first_edge[i + 1] += first_edge[i];
    }

    core::FixedArray<u32> edges(_links.size());
    core::FixedArray<u32> next_edge(node_count);
    for(const BlueprintLink& link : _links) {
        edges[first_edge[link.src_node] + next_edge[link.src_node]++] = link.dst_node;
    }

    core::ScratchVector<u32> order(node_count);
    for(usize i = 0; i != node_count; ++i) {
        if(!in_degrees[i]) {
            order.push_back(u32(i));
        }
    }

    for(usize i = 0; i != order.size(); ++i) {
        for(u32 e = first_edge[order[i]]; e != first_edge[order[i] + 1]; ++e) {
            if(!--in_degrees[edges[e]]) {
                order.push_back(edges[e]);
            }
        }
    }

    y_debug_assert(order.size() == node_count); // return?

    core::Vector<std::unique_ptr<BlueprintNode>> nodes;
    nodes.set_min_capacity(node_count);
    for(usize i = 0; i != node_count; ++i) {
        nodes << std::move(_nodes[order[i]]);
        next_edge[order[i]] = u32(i);
    }
    _nodes = std::move(nodes);

    for(BlueprintLink& link : _links) {
        link.src_node = next_edge[link.src_node];
        link.dst_node = next_edge[link.dst_node];
    }
}

void Blueprint::remove_link(const BlueprintNode* dst, usize dst_pin) {
    y_profile();

    const usize dst_index = find_node_index(dst);
    y_debug_assert(dst_index < _nodes.size());
    y_debug_assert(dst_pin < dst->input_pins().size());

    for(usize i = 0; i != _links.size(); ++i) {
        if(_links[i].dst_node == dst_index && _links[i].dst_pin == dst_pin) {
            _links.erase_unordered(_links.begin() + i);
            break;
        }
    }

    resolve_generic_types();
}

void Blueprint::clear_links() {
    y_profile();

    _links.make_empty();
    resolve_generic_types();
}

void Blueprint::resolve_generic_types() {
    y_profile();

    for(const auto& node : _nodes) {
        node->set_generic_type(nullptr);
    }

    for(bool changed = true; changed;) {
        changed = false;
        for(const BlueprintLink& link : _links) {
            changed |= resolve_generic_link(_nodes[link.src_node].get(), link.src_pin, _nodes[link.dst_node].get(), link.dst_pin);
        }
    }
}

core::Result<BlueprintInstance, BlueprintError> Blueprint::create_instance() const {
    y_profile();

    for(const BlueprintLink& link : _links) {
        if(link.src_node >= _nodes.size() || link.dst_node >= _nodes.size()) {
            return core::Err(BlueprintError{std::min<usize>(link.dst_node, _nodes.size()), core::String("Link references an invalid node")});
        }
        if(link.src_node >= link.dst_node) {
            return core::Err(BlueprintError{link.dst_node, core::String("Link breaks execution order")});
        }
    }

    BlueprintInstance instance;
    instance._nodes = clone_nodes();

    for(const BlueprintLink& link : _links) {
        if(link.src_pin >= instance._nodes[link.src_node]->output_pins().size() || link.dst_pin >= instance._nodes[link.dst_node]->input_pins().size()) {
            return core::Err(BlueprintError{link.dst_node, core::String("Link references an invalid pin")});
        }
    }

    for(bool changed = true; changed;) {
        changed = false;
        for(const BlueprintLink& link : _links) {
            changed |= resolve_generic_link(instance._nodes[link.src_node].get(), link.src_pin, instance._nodes[link.dst_node].get(), link.dst_pin);
        }
    }

    for(usize i = 0; i != instance._nodes.size(); ++i) {
        const BlueprintNode* node = instance._nodes[i].get();
        if(node->has_generic_pin() && !node->generic_type()) {
            return core::Err(BlueprintError{i, core::String("Unresolved generic type")});
        }
    }

    for(const BlueprintLink& link : _links) {
        const BlueprintNode* src = instance._nodes[link.src_node].get();
        BlueprintNode* dst = instance._nodes[link.dst_node].get();
        if(!are_blueprint_types_compatible(src->output_pins()[link.src_pin].type, dst->input_pins()[link.dst_pin].type)) {
            return core::Err(BlueprintError{link.dst_node, core::String("Link connects incompatible types")});
        }
        dst->set_input(link.dst_pin, src->output_ptr(link.src_pin));
    }

    return core::Ok(std::move(instance));
}

core::Vector<std::unique_ptr<BlueprintNode>> Blueprint::clone_nodes() const {
    core::Vector<std::unique_ptr<BlueprintNode>> nodes;
    nodes.set_min_capacity(_nodes.size());

    for(const auto& node : _nodes) {
        nodes.emplace_back(node->clone());
    }

    if(!_nested.is_empty()) {
        for(const auto& node : nodes) {
            if(auto* nested = dynamic_cast<NestedBlueprintNode*>(node.get()); nested && !nested->blueprint().is_loaded()) {
                if(const auto it = std::find(_nested.begin(), _nested.end(), nested->blueprint()); it != _nested.end() && it->is_loaded()) {
                    nested->set_blueprint(*it);
                }
            }
        }
    }

    return nodes;
}

Blueprint Blueprint::clone() const {
    y_profile();

    Blueprint blueprint;
    blueprint._nodes = clone_nodes();
    blueprint._links = core::Vector<BlueprintLink>(_links);
    blueprint._nested = core::Vector<AssetPtr<Blueprint>>(_nested);
    blueprint.resolve_generic_types();
    return blueprint;
}

bool Blueprint::remove_invalid_links() {
    y_profile();

    bool removed = false;
    for(usize i = 0; i != _links.size(); ++i) {
        const BlueprintLink& link = _links[i];
        const bool is_valid =
            link.src_node < _nodes.size() && link.dst_node < _nodes.size() &&
            link.src_pin < _nodes[link.src_node]->output_pins().size() &&
            link.dst_pin < _nodes[link.dst_node]->input_pins().size()
        ;
        if(!is_valid) {
            _links.erase_unordered(_links.begin() + i);
            --i;
            removed = true;
        }
    }

    return removed;
}

}
