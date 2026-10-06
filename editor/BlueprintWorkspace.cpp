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

#include "editor.h"
#include "ThumbnailRenderer.h"
#include "UiManager.h"

#include <yave/assets/AssetLoader.h>
#include <yave/assets/AssetStore.h>
#include <yave/blueprints/Blueprint.h>
#include <yave/blueprints/blueprint_nodes.h>
#include <yave/blueprints/NestedBlueprintNode.h>
#include <yave/utils/FileSystemModel.h>

#include <y/io2/Buffer.h>
#include <y/serde3/archives.h>
#include <y/utils/format.h>
#include <y/utils/log.h>

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

static core::Result<Blueprint> read_blueprint_data(AssetId id) {
    if(id == AssetId::invalid_id()) {
        log_msg("Unable to load blueprint: no asset id", Log::Error);
        return core::Err();
    }

    const auto loaded = asset_loader().load_res<Blueprint>(id);
    if(!loaded) {
        log_msg("Unable to load blueprint", Log::Error);
        return core::Err();
    }

    Blueprint data = loaded.unwrap()->clone();
    if(data.remove_invalid_links()) {
        log_msg("Some links referenced invalid pins and were removed", Log::Warning);
    }

    return core::Ok(std::move(data));
}

static Blueprint create_blueprint(core::Span<std::unique_ptr<BlueprintNodeFactory>> factories, usize node_count = 20) {
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

    float param_value = 0.0f;
    auto set_params = [&](auto node) {
        const core::Span params = node->param_pins();
        for(usize i = 0; i != params.size(); ++i) {
            if(const auto* type = params[i].type; type && type->size % sizeof(float) == 0) {
                std::fill_n(static_cast<float*>(node->param_ptr(i)), type->size / sizeof(float), param_value += 1.0f);
            }
        }
        return node;
    };

    floats.emplace_back(blueprint.add_node(set_params(find_factory(factories, "Const float")->create_node())));
    vec2s.emplace_back(blueprint.add_node(set_params(find_factory(factories, "Const Vec2")->create_node())));
    vec3s.emplace_back(blueprint.add_node(set_params(find_factory(factories, "Const Vec3")->create_node())));
    vec4s.emplace_back(blueprint.add_node(set_params(find_factory(factories, "Const Vec4")->create_node())));

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

    add_create_decompose(vec2s, 2, "Vec2");
    add_create_decompose(vec3s, 3, "Vec3");
    add_create_decompose(vec4s, 4, "Vec4");

    add_vec_ops(vec2s, "Vec2");
    add_vec_ops(vec3s, "Vec3");
    add_vec_ops(vec4s, "Vec4");

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


BlueprintWorkspace::BlueprintWorkspace(AssetId id) : _id(id) {
    add_all_nodes(_node_factories);
    update_name();
    if(_id != AssetId::invalid_id()) {
        load();
    } else {
        _blueprint = create_blueprint(_node_factories);
    }
}

BlueprintWorkspace::~BlueprintWorkspace() {
}

std::string_view BlueprintWorkspace::name() const {
    return _name;
}

void BlueprintWorkspace::update() {
    if(auto res = _blueprint.create_instance()) {
        _result = core::Ok();
    } else {
        _result = core::Err(std::move(res.error()));
    }
}

void BlueprintWorkspace::save() {
    y_profile();

    if(_id == AssetId::invalid_id()) {
        log_msg("Unable to save blueprint: no asset id", Log::Error);
        return;
    }

    io2::Buffer buffer;
    {
        serde3::WritableArchive arc(buffer);
        if(const auto res = arc.serialize(_blueprint); res.is_error()) {
            log_msg("Unable to serialize blueprint", Log::Error);
            return;
        }
        buffer.reset();
    }

    const auto nested = _blueprint.nested_blueprints();
    auto refs = core::Vector<AssetId>::with_capacity(nested.size());
    for(const auto& blueprint : nested) {
        if(blueprint.id() != AssetId::invalid_id()) {
            refs << blueprint.id();
        }
    }

    if(const auto res = asset_store().write(_id, buffer, refs); res.is_error()) {
        log_msg(fmt("Unable to write blueprint, error: {}", res.error()), Log::Error);
        return;
    }

    notify_asset_saved(_id);

    log_msg("Blueprint saved");
}

void BlueprintWorkspace::load() {
    y_profile();

    if(auto data = read_blueprint_data(_id)) {
        _selected_node = nullptr;
        _blueprint = std::move(data.unwrap());
        update_name();

        log_msg("Blueprint loaded");
    }
}

bool BlueprintWorkspace::add_blueprint(AssetId id) {
    y_profile();

    if(auto data = read_blueprint_data(id)) {
        Blueprint new_data = std::move(data.unwrap());
        if(new_data.contains_nested(_id)) {
            log_msg("A blueprint can not be nested in itself", Log::Error);
            return false;
        }
        _blueprint.add_blueprint(std::move(new_data));
        return true;
    }

    return false;
}

const BlueprintNode* BlueprintWorkspace::add_nested_blueprint(AssetId id) {
    y_profile();

    if(id == _id) {
        log_msg("A blueprint can not be nested in itself", Log::Error);
        return nullptr;
    }

    const auto loaded = asset_loader().load_res<Blueprint>(id);
    if(!loaded) {
        log_msg("Unable to load nested blueprint", Log::Error);
        return nullptr;
    }

    if(loaded.unwrap()->contains_nested(_id)) {
        log_msg("A blueprint can not be nested in itself", Log::Error);
        return nullptr;
    }

    core::String name = "Blueprint";
    if(auto full_name = asset_store().name(id)) {
        name = asset_store().filesystem()->filename(full_name.unwrap());
    }

    return _blueprint.add_node(std::make_unique<NestedBlueprintNode>(std::move(name), loaded.unwrap()));
}

AssetId BlueprintWorkspace::asset_id() const {
    return _id;
}

Blueprint& BlueprintWorkspace::blueprint() {
    return _blueprint;
}

const Blueprint& BlueprintWorkspace::blueprint() const {
    return _blueprint;
}

const core::Result<void, BlueprintError>& BlueprintWorkspace::result() const {
    return _result;
}

core::Span<std::unique_ptr<BlueprintNodeFactory>> BlueprintWorkspace::node_factories() const {
    return _node_factories;
}

BlueprintNode* BlueprintWorkspace::selected_node() const {
    return _selected_node;
}

void BlueprintWorkspace::set_selected_node(BlueprintNode* node) {
    _selected_node = node;
}

void BlueprintWorkspace::update_name() {
    _name = ICON_FA_PROJECT_DIAGRAM " Blueprint";
    if(_id != AssetId::invalid_id()) {
        if(auto name = asset_store().name(_id)) {
            _name = fmt("{} {}", ICON_FA_PROJECT_DIAGRAM, asset_store().filesystem()->filename(name.unwrap()));
        }
    }
}

}
