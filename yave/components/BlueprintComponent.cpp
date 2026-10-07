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

#include "BlueprintComponent.h"

#include <yave/assets/AssetLoader.h>
#include <yave/ecs/ComponentInspector.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static std::string_view error_node_name(const BlueprintError& error) {
    return error.node ? std::string_view(error.node->name()) : "unknown";
}

BlueprintComponent::BlueprintComponent(const AssetPtr<Blueprint>& blueprint) : _blueprint(blueprint) {
}

const AssetPtr<Blueprint>& BlueprintComponent::blueprint() const {
    return _blueprint;
}

const BlueprintInstance* BlueprintComponent::instance() const {
    return _instance.get();
}

bool BlueprintComponent::update_asset_loading_status() {
    return !_blueprint.is_loading();
}

void BlueprintComponent::load_assets(AssetLoadingContext& loading_ctx) {
    _blueprint.load_async(loading_ctx);
}

void BlueprintComponent::subscribe_triggers(ecs::TriggerSubscriber& subscriber) const {
    _instance = nullptr;

    if(!_blueprint) {
        return;
    }

    auto instance = _blueprint->create_instance();
    if(instance.is_error()) {
        log_msg(fmt("Unable to create blueprint instance: {} (node \"{}\")", instance.error().error, error_node_name(instance.error())), Log::Error);
        return;
    }

    _instance = std::make_shared<BlueprintInstance>(std::move(instance.unwrap()));

    for(const BlueprintInstance::EntryPoint& entry_point : _instance->entry_points()) {
        entry_point.subscribe(subscriber);
    }
}

void BlueprintComponent::on_trigger(ecs::EntityWorld& world, ecs::EntityId id, ecs::TriggerTypeIndex type, const void* payload) const {
    if(!_instance) {
        return;
    }

    const BlueprintContext context = {&world, id};

    for(const BlueprintInstance::EntryPoint& entry_point : _instance->entry_points()) {
        if(entry_point.trigger_type != type) {
            continue;
        }

        if(const auto res = _instance->trigger(entry_point, payload, context);res.is_error()) {
            log_msg(fmt("Blueprint error: {} (node \"{}\")", res.error().error, error_node_name(res.error())), Log::Error);
        }
    }
}

void BlueprintComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Blueprint", _blueprint);
}

}

