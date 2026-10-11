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

#include <yave/assets/AssetStore.h>
#include <yave/blueprints/Blueprint.h>
#include <yave/blueprints/blueprint_nodes.h>
#include <yave/utils/FileSystemModel.h>

#include <y/io2/Buffer.h>
#include <y/serde3/archives.h>
#include <y/utils/format.h>
#include <y/utils/log.h>

#include <external/imgui/imgui.h>

namespace editor {


static core::Result<Blueprint> read_blueprint_data(AssetId id) {
    if(id == AssetId::invalid_id()) {
        log_msg("Unable to load blueprint: no asset id", Log::Error);
        return core::Err();
    }

    auto reader = asset_store().data(id);
    if(!reader) {
        log_msg("Unable to read blueprint", Log::Error);
        return core::Err();
    }

    Blueprint data;
    if(const auto res = serde3::ReadableArchive(*reader.unwrap()).deserialize(data); res.is_error()) {
        log_msg(fmt("Unable to deserialize blueprint: {}", serde3::error_msg(res)), Log::Error);
        return core::Err();
    }

    return core::Ok(std::move(data));
}




BlueprintWorkspace::BlueprintWorkspace(AssetId id) : _id(id) {
    add_all_nodes(_node_factories);
    update_name();
    if(_id != AssetId::invalid_id()) {
        load();
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

    if(const auto res = asset_store().write(_id, buffer, {}); res.is_error()) {
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
        _result = core::Ok();
        _blueprint = std::move(data.unwrap());
        update_name();

        log_msg("Blueprint loaded");
    }
}

bool BlueprintWorkspace::add_blueprint(AssetId id) {
    y_profile();

    if(auto data = read_blueprint_data(id)) {
        _blueprint.add_blueprint(std::move(data.unwrap()));
        return true;
    }

    return false;
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
