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

BlueprintWorkspace::BlueprintWorkspace() {
    _blueprint.add_node(LambdaBlueprintNodeBuilder<>("Add")
        .add_input<float>("a")
        .add_input<float>("b")
        .add_output<float>("out")
        .build([](float a, float b, float& out) { out = a + b; })->create_node()
    );

    _blueprint.add_node(LambdaBlueprintNodeBuilder<>("Multiply")
        .add_input<float>("a")
        .add_input<float>("b")
        .add_output<float>("out")
        .build([](float a, float b, float& out) { out = a * b; })->create_node()
    );

    _blueprint.add_node(LambdaBlueprintNodeBuilder<>("Negate")
        .add_input<float>("in")
        .add_output<float>("out")
        .build([](float in, float& out) { out = -in; })->create_node()
    );

    _blueprint.add_node(LambdaBlueprintNodeBuilder<>("Const")
        .add_output<float>("value")
        .build([](float& value) { value = 1.0f; })->create_node()
    );
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
