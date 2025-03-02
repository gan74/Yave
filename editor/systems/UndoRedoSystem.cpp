/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "UndoRedoSystem.h"

#include <yave/ecs/EntityWorld.h>
#include <yave/ecs/ComponentBox.h>

#include <y/io2/Buffer.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


namespace editor {


template<typename T>
static io2::Buffer serialize_to_buffer(const T& t) {
    io2::Buffer buffer;
    serde3::WritableArchive warc(buffer);
    warc.serialize(t).expected("Unable to serialize object");
    buffer.reset();
    return buffer;
}



class UndoRedoSystem::GetterInspector final : public ecs::TemplateComponentInspector<UndoRedoSystem::GetterInspector> {
    public:
        GetterInspector(core::SmallVector<Property, 8>& props) : _properties(props) {
        }

        template<typename T>
        void visit(const core::String& name, T& t) {
            // AssetPtr are not supported
            if constexpr(!std::is_same_v<T, GenericAssetPtr>) {
                _properties.emplace_back(name, t);
            }
        }

    private:
        core::SmallVector<Property, 8>& _properties;
};

class UndoRedoSystem::SetterInspector final : public ecs::TemplateComponentInspector<UndoRedoSystem::SetterInspector> {
    public:
        SetterInspector(core::Span<Property> props) : _properties(props) {
        }

        template<typename T>
        void visit(const core::String& name, T& t) {
            // AssetPtr are not supported
            if constexpr(!std::is_same_v<T, GenericAssetPtr>) {
                const auto it = std::find_if(_properties.begin(), _properties.end(), [&](const auto& p) { return p.name == name; });
                if(it != _properties.end()) {
                    if(const T* value = std::get_if<std::remove_cvref_t<T>>(&it->value)) {
                        t = *value;
                    } else {
                        log_msg(fmt("Unable to set property \"{}\"", name), Log::Error);
                    }
                }
            }
        }

    private:
        core::Span<Property> _properties;
};


void UndoRedoSystem::UndoState::undo(ecs::EntityWorld& world) {
    y_profile();

    for(const auto& [key, properties] : undo_properties) {
        SetterInspector inspector(properties);
        world.inspect_components(key.first, &inspector, key.second);
    }

    for(const ecs::EntityId id : removed_entities) {
        world.create_entity_with_id(id);
    }

    for(const ecs::EntityId id : added_entities) {
        world.remove_entity(id);
    }

    for(const auto& [id, comp] : removed_components) {
        comp->add_or_replace(world, id);
    }

    for(const auto& [id, comp] : added_components) {
        world.remove_component(id, comp->runtime_info().type_id);
    }
}

void UndoRedoSystem::UndoState::redo(ecs::EntityWorld& world) {
    y_profile();

    for(const auto& [key, properties] : redo_properties) {
        SetterInspector inspector(properties);
        world.inspect_components(key.first, &inspector, key.second);
    }

    for(const ecs::EntityId id : removed_entities) {
        world.remove_entity(id);
    }

    for(const ecs::EntityId id : added_entities) {
        world.create_entity_with_id(id);
    }

    for(const auto& [id, comp] : removed_components) {
        world.remove_component(id, comp->runtime_info().type_id);
    }

    for(const auto& [id, comp] : added_components) {
        comp->add_or_replace(world, id);
    }
}


UndoRedoSystem::UndoRedoSystem() : ecs::System("UndoRedoSystem") {
}

void UndoRedoSystem::reset() {
    _states.clear();
    _top = 0;
    _do_undo = false;
    _do_redo = false;
    take_snapshot();
}

void UndoRedoSystem::setup(ecs::SystemScheduler& sched) {
    sched.schedule(ecs::SystemSchedule::TickSequential, "TickSeq", [this]() {
        if(!_do_undo && !_do_redo) {
            UndoState state;
            for(const ecs::EntityId id : world().pending_deletions()) {
                state.removed_entities << id;
            }

            for(const ecs::EntityId id : world().recently_added()) {
                state.added_entities << id;
            }

            for(const ecs::ComponentContainerBase* container : world().component_containers()) {
                if(!container->runtime_info().is_inspectable) {
                    continue;
                }

                const ecs::ComponentTypeIndex type_id = container->type_id();

                for(const ecs::EntityId id : container->mutated_ids()) {
                    if(_snapshot->exists(id) && _snapshot->has_component(id, type_id)) {
                        const ComponentKey key{id, type_id};
                        GetterInspector inspector(state.redo_properties.emplace_back(key, core::Vector<Property>()).second);
                        world().inspect_components(id, &inspector, container->type_id());
                    } else {
                        std::unique_ptr<ecs::ComponentBoxBase> comp = world().create_box_from_component(id, type_id);
                        y_debug_assert(comp);
                        state.added_components.emplace_back(id, std::move(comp));
                    }
                }

                for(const ecs::EntityId id : container->pending_deletions()) {
                    state.removed_components.emplace_back(id, world().create_box_from_component(id, type_id));
                }
            }

            push_state(std::move(state));
        }

        if(_do_undo) {
            _do_undo = false;
            if(_top) {
                --_top;
                _states[_top].undo(world());
                _states[_top].undo(*_snapshot);
                _snapshot->process_deferred_changes();
            } else {
                log_msg("Nothing to undo", Log::Warning);
            }
        }

        if(_do_redo) {
            _do_redo = false;
            if(_top != _states.size()) {
                _states[_top].redo(world());
                _states[_top].redo(*_snapshot);
                _snapshot->process_deferred_changes();
                ++_top;
            } else {
                log_msg("Nothing to redo", Log::Warning);
            }

        }
    });
}

void UndoRedoSystem::undo() {
    _do_undo = true;
}

void UndoRedoSystem::redo() {
    _do_redo = true;
}


void UndoRedoSystem::take_snapshot() {
    y_profile();

    io2::Buffer buffer;
    serde3::WritableArchive warc(buffer);
    world().save_state(warc).expected("Unable to serialize world");
    buffer.reset();

    serde3::ReadableArchive rarc(buffer);
    _snapshot = std::make_unique<ecs::EntityWorld>();
    _snapshot->load_state(rarc).expected("Unable to serialize world");
}

void UndoRedoSystem::push_state(UndoState state) {
    y_profile();

    {
        y_profile_zone("resolving changed properties");
        y_debug_assert(state.undo_properties.is_empty());

        for(const auto& [key, properties] : state.redo_properties) {
            GetterInspector inspector(state.undo_properties.emplace_back(key, core::Vector<Property>()).second);
            _snapshot->inspect_components(key.first, &inspector, key.second);
        }

        auto value_equal = [](const PropertyValue& a, const PropertyValue& b) {
            /*const auto* tra = std::get_if<math::Transform<>>(&a);
            const auto* trb = std::get_if<math::Transform<>>(&b);
            if(tra && trb) {
                Y_TODO(better heuristic?)
                return true;
            }*/
            return a == b;
        };


        y_debug_assert(state.redo_properties.size() == state.undo_properties.size());
        for(usize k = 0; k < state.redo_properties.size(); ++k) {
            auto&& [key, redo_properties] = state.redo_properties[k];
            auto&& undo_properties = state.undo_properties[k].second;
            y_debug_assert(state.undo_properties[k].first == key);
            y_debug_assert(redo_properties.size() == undo_properties.size());

            for(usize i = 0; i < redo_properties.size(); ++i) {
                const Property& redo_prop = redo_properties[i];
                const Property& undo_prop = undo_properties[i];

                y_debug_assert(redo_prop.name == undo_prop.name);
                if(value_equal(redo_prop.value, undo_prop.value)) {
                    redo_properties.erase_unordered(redo_properties.begin() + i);
                    undo_properties.erase_unordered(undo_properties.begin() + i);
                    --i;
                }
            }

            y_debug_assert(redo_properties.size() == undo_properties.size());
            if(redo_properties.is_empty() && undo_properties.is_empty()) {
                state.redo_properties.erase_unordered(state.redo_properties.begin() + k);
                state.undo_properties.erase_unordered(state.undo_properties.begin() + k);
                --k;
            }
        }
    }

    const bool has_entity_changes =
        !state.added_components.is_empty() ||
        !state.removed_components.is_empty() ||
        !state.added_entities.is_empty() ||
        !state.removed_entities.is_empty()
    ;

    y_debug_assert(state.redo_properties.size() == state.undo_properties.size());
    if(!has_entity_changes && state.redo_properties.is_empty() && state.undo_properties.is_empty()) {
        return;
    }

    {
        y_profile_zone("updating snapshot");
        y_debug_assert(_snapshot);
        state.redo(*_snapshot);
        _snapshot->process_deferred_changes();
    }

    if(_states.size() != _top) {
        do {
            _states.pop();
        } while(_states.size() != _top);
    } else if(!has_entity_changes && !_states.is_empty()) {
        UndoState& last = _states.last();
        if(last.created.elapsed() < merge_time_threshold && last.redo_properties.size() == state.redo_properties.size()) {
            bool match = true;
            for(usize i = 0; match && i != state.redo_properties.size(); ++i) {
                match = match && (last.redo_properties[i].first == state.redo_properties[i].first);
                match = match && (last.redo_properties[i].second.size() == state.redo_properties[i].second.size());
                for(usize k = 0; match && k != last.redo_properties[i].second.size(); ++k) {
                    match = match && last.redo_properties[i].second[k].name == state.redo_properties[i].second[k].name;
                }
            }

            if(match) {
                last.redo_properties.swap(state.redo_properties);
                last.created.reset();
                return;
            }
        }
    }


    ++_top;
    _states.emplace_back(std::move(state));
}

}

