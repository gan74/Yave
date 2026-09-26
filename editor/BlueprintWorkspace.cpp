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

#include "BlueprintWorkspace.h"

#include <yave/blueprints/BlueprintNodeBuilder.h>

#include <external/imgui/imgui.h>

namespace editor {

static Blueprint create_blueprint(usize node_count = 1000) {
    y_profile();

    const auto const_factory = LambdaBlueprintNodeBuilder<>("Const")
        .add_output<float>("value")
        .build([](float& value) { value = 1.0f; })
    ;

    const auto negate_factory = LambdaBlueprintNodeBuilder<>("Negate")
        .add_input<float>("in")
        .add_output<float>("out")
        .build([](float in, float& out) { out = -in; })
    ;

    const auto add_factory = LambdaBlueprintNodeBuilder<>("Add")
        .add_input<float>("a")
        .add_input<float>("b")
        .add_output<float>("out")
        .build([](float a, float b, float& out) { out = a + b; })
    ;

    const auto mul_factory = LambdaBlueprintNodeBuilder<>("Multiply")
        .add_input<float>("a")
        .add_input<float>("b")
        .add_output<float>("out")
        .build([](float a, float b, float& out) { out = a * b; })
    ;

    auto nodes = core::Vector<const BlueprintNode*>::with_capacity(node_count);

    Blueprint blueprint;

    const usize const_count = node_count / 10;
    for(usize i = 0; i != const_count; ++i) {
        nodes.emplace_back(blueprint.add_node(const_factory->create_node()));
    }

    for(usize i = const_count; i != node_count; ++i) {
        const usize a = i - 1;
        const usize b = (i - const_count) % const_count;
        const usize kind = i % 3;

        if(kind == 0) {
            const BlueprintNode* node = blueprint.add_node(negate_factory->create_node());
            blueprint.add_link(nodes[a], 0, node, 0);
            nodes.emplace_back(node);
        } else if(kind == 1) {
            const BlueprintNode* node = blueprint.add_node(add_factory->create_node());
            blueprint.add_link(nodes[a], 0, node, 0);
            blueprint.add_link(nodes[b], 0, node, 1);
            nodes.emplace_back(node);
        } else {
            const BlueprintNode* node = blueprint.add_node(mul_factory->create_node());
            blueprint.add_link(nodes[b], 0, node, 0);
            blueprint.add_link(nodes[a], 0, node, 1);
            nodes.emplace_back(node);
        }
    }

    return blueprint;
}



BlueprintWorkspace::BlueprintWorkspace() : _blueprint(create_blueprint()) {
}

BlueprintWorkspace::~BlueprintWorkspace() {
}

std::string_view BlueprintWorkspace::name() const {
    return ICON_FA_PROJECT_DIAGRAM " Blueprint";
}

void BlueprintWorkspace::update() {
    _blueprint.eval();
}

void BlueprintWorkspace::save() {
}

void BlueprintWorkspace::load() {
}

Blueprint& BlueprintWorkspace::blueprint() {
    return _blueprint;
}

const Blueprint& BlueprintWorkspace::blueprint() const {
    return _blueprint;
}

BlueprintNode* BlueprintWorkspace::selected_node() const {
    return _selected_node;
}

void BlueprintWorkspace::set_selected_node(BlueprintNode* node) {
    _selected_node = node;
}

}
