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

#include "WorldWorkspace.h"

#include "editor.h"

#include <yave/assets/AssetLoader.h>
#include <yave/systems/SceneSystem.h>
#include <yave/systems/JoltPhysicsSystem.h>

#include <y/io2/File.h>
#include <y/serde3/archives.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

WorldWorkspace::WorldWorkspace(AssetLoader& loader) :
        _world(std::make_unique<EditorWorld>(loader)),
        _job_system(std::make_unique<concurrent::JobSystem>()),
        _debug_drawer(std::make_unique<DirectDraw>()) {

    create_default_scene_view();
}

WorldWorkspace::~WorldWorkspace() {
    _scene_view = nullptr;
    _default_scene_view = {};
    _world = nullptr;
    _debug_drawer = nullptr;
    _job_system = nullptr;
}

std::string_view WorldWorkspace::name() const {
    return "World";
}

void WorldWorkspace::update() {
    y_profile();

    _world->tick(*_job_system);
    _world->process_deferred_changes();
}

void WorldWorkspace::post_update() {
    y_profile();

    process_deferred_actions();

    if(JoltPhysicsSystem* jolt = _world->find_system<JoltPhysicsSystem>()) {
        jolt->set_debug_drawer(_debug_drawer.get());
        jolt->set_debug_draw_static(app_settings().debug.display_static_colliders);
        jolt->set_debug_draw_movable(app_settings().debug.display_movable_colliders);
    }
}

void WorldWorkspace::save() {
    _deferred_actions |= Save;
}

void WorldWorkspace::load() {
    _deferred_actions |= Load;
}

EditorWorld& WorldWorkspace::world() {
    return *_world;
}

const EditorWorld& WorldWorkspace::world() const {
    return *_world;
}

const Scene& WorldWorkspace::scene() const {
    const Scene* sce = _world->find_system<SceneSystem>()->scene();
    y_debug_assert(sce);
    return *sce;
}

void WorldWorkspace::set_scene_view(SceneView* scene) {
    if(!scene) {
        _scene_view = &_default_scene_view;
    } else {
        _scene_view = scene;
    }
}

void WorldWorkspace::unset_scene_view(SceneView* scene) {
    if(_scene_view == scene) {
        set_scene_view(nullptr);
    }
}

const SceneView& WorldWorkspace::scene_view() const {
    y_debug_assert(_scene_view);
    return *_scene_view;
}

concurrent::JobSystem& WorldWorkspace::job_system() {
    return *_job_system;
}

DirectDraw& WorldWorkspace::debug_drawer() {
    return *_debug_drawer;
}

void WorldWorkspace::create_default_scene_view() {
    _default_scene_view = SceneView(&scene());
    set_scene_view(nullptr);
}

void WorldWorkspace::save_world_deferred() {
    y_profile();

    auto file = io2::File::create(app_settings().editor.world_file);
    if(!file) {
        log_msg("Unable to open world file", Log::Error);
        return;
    }

    serde3::WritableArchive arc(file.unwrap());
    if(auto r = _world->save_state(arc); !r) {
        log_msg(fmt("Unable to save world: {}", serde3::error_msg(r.error())), Log::Error);
        return;
    }

    log_msg("World saved");
}

void WorldWorkspace::load_world_deferred() {
    y_profile();

    auto file = io2::File::open(app_settings().editor.world_file);
    if(!file) {
        log_msg("Unable to open world file", Log::Error);
        return;
    }

    auto world = std::make_unique<EditorWorld>(asset_loader());

    serde3::ReadableArchive arc(file.unwrap(), serde3::DeserializationFlags::DontPropagatePolyFailure);
    if(auto r = world->load_state(arc); !r) {
        const char* member_name = r.error().member ? r.error().member : "unknown member";
        log_msg(fmt("Unable to load world: {} (for {})", serde3::error_msg(r.error()), member_name), Log::Error);
        return;
    } else if(r.unwrap() == serde3::Success::Partial) {
        log_msg("World was only partially loaded", Log::Warning);
    }

    _world = std::move(world);
    create_default_scene_view();

    log_msg("World loaded");
}

void WorldWorkspace::process_deferred_actions() {
    if(_deferred_actions & Save) {
        save_world_deferred();
    }

    if(_deferred_actions & Load) {
        load_world_deferred();
    }

    if(_deferred_actions & New) {
        _world = std::make_unique<EditorWorld>(asset_loader());
        create_default_scene_view();
    }

    _deferred_actions = None;
}

}
