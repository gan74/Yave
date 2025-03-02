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
#ifndef EDITOR_SYSTEMS_UNDOREDOSYSTEM_H
#define EDITOR_SYSTEMS_UNDOREDOSYSTEM_H

#include <yave/ecs/System.h>
#include <yave/assets/AssetId.h>
#include <yave/assets/AssetPtr.h>

#include <y/core/Chrono.h>
#include <y/core/Vector.h>
#include <y/core/HashMap.h>
#include <variant>

namespace editor {

class UndoRedoSystem : public ecs::System {
    using ComponentKey = std::pair<ecs::EntityId, ecs::ComponentTypeIndex>;

    class GetterInspector;
    class SetterInspector;

    using PropertyValue = std::variant<
        core::String,
        math::Transform<>,
        math::Vec3,
        float,
        u32,
        bool,
        ecs::EntityId,
        AssetId,
        GenericAssetPtr
    >;

    struct Property {
        core::String name;
        PropertyValue value;
    };

    struct UndoState {
        core::Vector<std::pair<ComponentKey, core::SmallVector<Property, 8>>> redo_properties;
        core::Vector<std::pair<ComponentKey, core::SmallVector<Property, 8>>> undo_properties;

        core::Vector<std::pair<ecs::EntityId, std::unique_ptr<ecs::ComponentBoxBase>>> removed_components;
        core::Vector<std::pair<ecs::EntityId, std::unique_ptr<ecs::ComponentBoxBase>>> added_components;

        core::Vector<std::pair<ecs::EntityId, ecs::EntityId>> removed_entities;
        core::Vector<ecs::EntityId> added_entities;

        core::Vector<std::tuple<ecs::EntityId, ecs::EntityId, ecs::EntityId>> parent_changed;

        core::StopWatch created;

        void undo(ecs::EntityWorld& world);
        void redo(ecs::EntityWorld& world);
    };

    public:
        static constexpr auto merge_time_threshold = core::Duration::seconds(0.5);

        UndoRedoSystem();

        void reset() override;
        void setup(ecs::SystemScheduler& sched) override;

        void undo();
        void redo();

        usize stack_top() const {
            return _top;
        }

        core::Span<UndoState> undo_states() const {
            return _states;
        }

    private:
        void take_snapshot();
        void push_state(UndoState state);

        core::Vector<UndoState> _states;
        usize _top = 0;

        std::unique_ptr<ecs::EntityWorld> _snapshot;

        bool _do_undo = false;
        bool _do_redo = false;
};

}

#endif // EDITOR_SYSTEMS_UNDOREDOSYSTEM_H

