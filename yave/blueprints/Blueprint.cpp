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

static void propagate_generic_types(core::Span<std::unique_ptr<BlueprintNode>> nodes, core::Span<BlueprintLink> links) {
    for(bool changed = true; changed;) {
        changed = false;
        for(const BlueprintLink& link : links) {
            changed |= resolve_generic_link(nodes[link.src_node].get(), link.src_pin, nodes[link.dst_node].get(), link.dst_pin);
        }
    }
}

static bool is_link_in_range(core::Span<std::unique_ptr<BlueprintNode>> nodes, const BlueprintLink& link) {
    return
        link.src_node < nodes.size() && link.dst_node < nodes.size() &&
        link.src_pin < nodes[link.src_node]->output_pins().size() &&
        link.dst_pin < nodes[link.dst_node]->input_pins().size()
    ;
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

const BlueprintLink* Blueprint::find_link(const BlueprintNode* dst, usize dst_pin) const {
    for(const BlueprintLink& link : _links) {
        if(_nodes[link.dst_node].get() == dst && link.dst_pin == dst_pin) {
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

    if(find_node_index(dst) == usize(-1)) {
        return false;
    }

    const BlueprintParamType* src_type = src_outputs[src_pin].type;
    const BlueprintParamType* dst_type = dst_inputs[dst_pin].type;
    if(src_type && dst_type && src_type != dst_type) {
        return false;
    }

    // Linking a node downstream of dst back into dst would create a cycle
    return !downstream_nodes(dst)[src_index];
}

void Blueprint::add_link(const BlueprintNode* src, usize src_pin, const BlueprintNode* dst, usize dst_pin) {
    y_profile();

    y_debug_assert(is_link_valid(src, src_pin, dst, dst_pin));

    const usize src_index = find_node_index(src);
    const usize dst_index = find_node_index(dst);

    erase_link(dst, dst_pin);
    _links << BlueprintLink{u32(src_index), u32(src_pin), u32(dst_index), u32(dst_pin)};

    if(src_index > dst_index) {
        move_downstream_after(dst, src_index);
    }

    resolve_generic_types();
}

// Moves node and everything that depends on it after index, keeping their relative order.
// Valid as long as nothing up to index depends on node (which is_link_valid ensures).
void Blueprint::move_downstream_after(const BlueprintNode* node, usize index) {
    y_profile();

    const usize node_count = _nodes.size();
    const core::FixedArray<bool> downstream = downstream_nodes(node);

    core::ScratchVector<u32> order(node_count);
    for(usize i = 0; i <= index; ++i) {
        if(!downstream[i]) {
            order.push_back(u32(i));
        }
    }
    for(usize i = 0; i != node_count; ++i) {
        if(downstream[i] || i > index) {
            order.push_back(u32(i));
        }
    }

    core::FixedArray<u32> new_index(node_count);
    core::Vector<std::unique_ptr<BlueprintNode>> nodes;
    nodes.set_min_capacity(node_count);
    for(usize i = 0; i != node_count; ++i) {
        nodes << std::move(_nodes[order[i]]);
        new_index[order[i]] = u32(i);
    }
    _nodes = std::move(nodes);

    for(BlueprintLink& link : _links) {
        link.src_node = new_index[link.src_node];
        link.dst_node = new_index[link.dst_node];
    }
}

void Blueprint::remove_link(const BlueprintNode* dst, usize dst_pin) {
    y_profile();

    y_debug_assert(dst_pin < dst->input_pins().size());

    erase_link(dst, dst_pin);
    resolve_generic_types();
}

void Blueprint::erase_link(const BlueprintNode* dst, usize dst_pin) {
    for(usize i = 0; i != _links.size(); ++i) {
        if(_nodes[_links[i].dst_node].get() == dst && _links[i].dst_pin == dst_pin) {
            _links.erase_unordered(_links.begin() + i);
            return;
        }
    }
}

void Blueprint::clear_links() {
    y_profile();

    _links.make_empty();
    resolve_generic_types();
}

void Blueprint::resolve_generic_types() {
    y_profile();

    for(const auto& node : _nodes) {
        if(node->generic_type()) {
            node->set_generic_type(nullptr);
        }
    }

    propagate_generic_types(_nodes, _links);
}

core::Result<BlueprintInstance, BlueprintError> Blueprint::create_instance() const {
    y_profile();

    BlueprintInstance instance;
    instance._nodes = clone_nodes();

    // Check against the clones: cloning can load nested blueprints, which changes their pins
    for(const BlueprintLink& link : _links) {
        if(!is_link_in_range(instance._nodes, link)) {
            return core::Err(BlueprintError{std::min<usize>(link.dst_node, _nodes.size()), core::String("Link references an invalid node or pin")});
        }
        if(link.src_node >= link.dst_node) {
            return core::Err(BlueprintError{link.dst_node, core::String("Link breaks execution order")});
        }
    }

    propagate_generic_types(instance._nodes, _links);

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

    const usize node_count = instance._nodes.size();

    core::FixedArray<core::FixedArray<bool>> downstream(node_count);
    core::ScratchPad<bool> depends_on_any(node_count, false);
    for(usize e = 0; e != node_count; ++e) {
        if(_nodes[e]->is_entry_point()) {
            downstream[e] = downstream_nodes(_nodes[e].get());
            for(usize i = 0; i != node_count; ++i) {
                depends_on_any[i] |= downstream[e][i];
            }
        }
    }

    for(usize e = 0; e != node_count; ++e) {
        if(!_nodes[e]->is_entry_point()) {
            continue;
        }

        BlueprintInstance::EntryPoint& entry_point = instance._entry_points.emplace_back();
        entry_point.node = instance._nodes[e].get();
        for(usize i = 0; i != node_count; ++i) {
            if(downstream[e][i] || !depends_on_any[i]) {
                entry_point.nodes << instance._nodes[i].get();
            }
        }
    }

    return core::Ok(std::move(instance));
}

core::FixedArray<bool> Blueprint::downstream_nodes(const BlueprintNode* node) const {
    y_profile();

    const usize node_index = find_node_index(node);
    y_debug_assert(node_index < _nodes.size());

    // Nodes are sorted, so anything downstream comes after node
    core::FixedArray<bool> downstream(_nodes.size());
    downstream[node_index] = true;

    for(usize i = node_index + 1; i != _nodes.size(); ++i) {
        for(const BlueprintLink& link : _links) {
            if(link.dst_node == i && downstream[link.src_node]) {
                downstream[i] = true;
                break;
            }
        }
    }

    return downstream;
}

core::Vector<std::unique_ptr<BlueprintNode>> Blueprint::clone_nodes() const {
    core::Vector<std::unique_ptr<BlueprintNode>> nodes;
    nodes.set_min_capacity(_nodes.size());

    for(const auto& node : _nodes) {
        nodes.emplace_back(node->clone());
    }

    for(const auto& node : nodes) {
        if(auto* nested = dynamic_cast<NestedBlueprintNode*>(node.get()); nested && !nested->blueprint().is_loaded()) {
            if(const auto it = std::find(_nested.begin(), _nested.end(), nested->blueprint()); it != _nested.end() && it->is_loaded()) {
                nested->set_blueprint(*it);
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
        if(!is_link_in_range(_nodes, _links[i])) {
            _links.erase_unordered(_links.begin() + i);
            --i;
            removed = true;
        }
    }

    return removed;
}

}
