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

#include <yave/blueprints/blueprint_nodes.h>

#include <y/utils/format.h>

#include <external/imgui/imgui.h>

namespace editor {

static BlueprintNodeFactory* find_factory(core::Span<std::unique_ptr<BlueprintNodeFactory>> factories, std::string_view name) {
    for(const auto& factory : factories) {
        if(factory->name() == name) {
            return factory.get();
        }
    }
    y_fatal("Unknown blueprint node factory '{}'", name);
}

static Blueprint create_blueprint(core::Span<std::unique_ptr<BlueprintNodeFactory>> factories, usize node_count = 200) {
    y_profile();

    Blueprint blueprint;
    core::Vector<const BlueprintNode*> floats;
    core::Vector<const BlueprintNode*> vec2s;
    core::Vector<const BlueprintNode*> vec3s;
    core::Vector<const BlueprintNode*> vec4s;

    auto add_unary = [&](core::Vector<const BlueprintNode*>& srcs, std::string_view name) {
        const BlueprintNode* node = blueprint.add_node(find_factory(factories, name)->create_node());
        blueprint.add_link(srcs.last(), 0, node, 0);
        srcs.emplace_back(node);
    };

    auto add_binary = [&](core::Vector<const BlueprintNode*>& srcs, std::string_view name) {
        const BlueprintNode* node = blueprint.add_node(find_factory(factories, name)->create_node());
        blueprint.add_link(srcs[srcs.size() - 1], 0, node, 0);
        blueprint.add_link(srcs[srcs.size() / 2], 0, node, 1);
        srcs.emplace_back(node);
    };

    // Divisor is always the type's Const (value 1) so we never divide by zero.
    auto add_divide = [&](core::Vector<const BlueprintNode*>& srcs, std::string_view name) {
        const BlueprintNode* node = blueprint.add_node(find_factory(factories, name)->create_node());
        blueprint.add_link(srcs.last(), 0, node, 0);
        blueprint.add_link(srcs[0], 0, node, 1);
        srcs.emplace_back(node);
    };

    auto add_to_floats_unary = [&](core::Vector<const BlueprintNode*>& srcs, std::string_view name) {
        const BlueprintNode* node = blueprint.add_node(find_factory(factories, name)->create_node());
        blueprint.add_link(srcs.last(), 0, node, 0);
        floats.emplace_back(node);
    };

    auto add_to_floats_binary = [&](core::Vector<const BlueprintNode*>& srcs, std::string_view name) {
        const BlueprintNode* node = blueprint.add_node(find_factory(factories, name)->create_node());
        blueprint.add_link(srcs[srcs.size() - 1], 0, node, 0);
        blueprint.add_link(srcs[srcs.size() / 2], 0, node, 1);
        floats.emplace_back(node);
    };

    auto add_create_decompose = [&](core::Vector<const BlueprintNode*>& vecs, usize comps, std::string_view type_name) {
        const BlueprintNode* create = blueprint.add_node(find_factory(factories, fmt("Create {}", type_name))->create_node());
        for(usize c = 0; c != comps; ++c) {
            blueprint.add_link(floats[floats.size() - 1 - c], 0, create, c);
        }
        vecs.emplace_back(create);

        const BlueprintNode* decomp = blueprint.add_node(find_factory(factories, fmt("Decompose {}", type_name))->create_node());
        blueprint.add_link(create, 0, decomp, 0);
        floats.emplace_back(decomp);
    };

    auto add_vec_ops = [&](core::Vector<const BlueprintNode*>& srcs, std::string_view type_name) {
        add_unary(srcs, fmt("Normalize {}", type_name));
        add_unary(srcs, fmt("Abs {}", type_name));
        add_unary(srcs, fmt("Saturate {}", type_name));
        add_binary(srcs, fmt("Cross {}", type_name));
        add_to_floats_unary(srcs, fmt("Length {}", type_name));
        add_to_floats_binary(srcs, fmt("Dot {}", type_name));
    };

    floats.emplace_back(blueprint.add_node(find_factory(factories, "Const float")->create_node()));
    vec2s.emplace_back(blueprint.add_node(find_factory(factories, "Const Vec2")->create_node()));
    vec3s.emplace_back(blueprint.add_node(find_factory(factories, "Const Vec3")->create_node()));
    vec4s.emplace_back(blueprint.add_node(find_factory(factories, "Const Vec4")->create_node()));

    // One of each math op per type (also connects the seed consts).
    add_unary(floats, "Negate float");
    add_binary(floats, "Add float");
    add_binary(floats, "Multiply float");
    add_divide(floats, "Divide float");

    add_unary(vec2s, "Negate Vec2");
    add_binary(vec2s, "Add Vec2");
    add_binary(vec2s, "Multiply Vec2");
    add_divide(vec2s, "Divide Vec2");

    add_unary(vec3s, "Negate Vec3");
    add_binary(vec3s, "Add Vec3");
    add_binary(vec3s, "Multiply Vec3");
    add_divide(vec3s, "Divide Vec3");

    add_unary(vec4s, "Negate Vec4");
    add_binary(vec4s, "Add Vec4");
    add_binary(vec4s, "Multiply Vec4");
    add_divide(vec4s, "Divide Vec4");

    // Create / decompose for each vector type.
    add_create_decompose(vec2s, 2, "Vec2");
    add_create_decompose(vec3s, 3, "Vec3");
    add_create_decompose(vec4s, 4, "Vec4");

    add_vec_ops(vec2s, "Vec2");
    add_vec_ops(vec3s, "Vec3");
    add_vec_ops(vec4s, "Vec4");

    // Fill the rest with a rotating mix.
    for(usize i = 0; blueprint.all_nodes().size() < node_count; ++i) {
        const usize kind = i % 14;
        if(kind == 0) {
            add_unary(floats, "Negate float");
        } else if(kind == 1) {
            add_binary(floats, "Add float");
        } else if(kind == 2) {
            add_binary(floats, "Multiply float");
        } else if(kind == 3) {
            add_divide(floats, "Divide float");
        } else if(kind == 4) {
            add_binary(vec2s, "Add Vec2");
        } else if(kind == 5) {
            add_binary(vec3s, "Multiply Vec3");
        } else if(kind == 6) {
            add_unary(vec4s, "Negate Vec4");
        } else if(kind == 7) {
            add_create_decompose(vec2s, 2, "Vec2");
        } else if(kind == 8) {
            add_create_decompose(vec3s, 3, "Vec3");
        } else if(kind == 9) {
            add_create_decompose(vec4s, 4, "Vec4");
        } else if(kind == 10) {
            add_unary(vec2s, "Normalize Vec2");
        } else if(kind == 11) {
            add_to_floats_binary(vec3s, "Dot Vec3");
        } else if(kind == 12) {
            add_binary(vec3s, "Cross Vec3");
        } else {
            add_to_floats_unary(vec4s, "Length Vec4");
        }
    }

    return blueprint;
}


BlueprintWorkspace::BlueprintWorkspace() {
    add_all_nodes(_node_factories);
    _blueprint = create_blueprint(_node_factories);
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
