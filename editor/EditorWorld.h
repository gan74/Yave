/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef EDITOR_EDITORWORLD_H
#define EDITOR_EDITORWORLD_H

#include <editor/editor.h>

#include <yave/ecs/ComponentRuntimeInfo.h>
#include <yave/ecs/EntityWorld.h>

#include <editor/utils/ui.h>

namespace editor {

class EditorWorld : public ecs::EntityWorld {
    public:
        EditorWorld(AssetLoader& loader);

        void clear();
        void flush_reload();

        bool set_entity_name(ecs::EntityId id, std::string_view name);
        std::string_view entity_name(ecs::EntityId id) const;
        UiIcon entity_icon(ecs::EntityId id) const;

        template<typename... Args>
        ecs::EntityId create_named_entity(std::string_view name, Args&&... args) {
            const ecs::EntityId id = create_entity(y_fwd(args)...);
            y_always_assert(set_entity_name(id, name), "Unable to set entity name");
            return id;
        }

        ecs::EntityId add_prefab(std::string_view name);
        ecs::EntityId add_prefab(AssetId asset);


        bool has_selected_entities() const;
        usize selected_entity_count() const;
        ecs::EntityId selected_entity() const;
        core::Span<ecs::EntityId> selected_entities() const;
        bool is_selected(ecs::EntityId id) const;

        void set_selected(ecs::EntityId id);
        void toggle_selected(ecs::EntityId id, bool reset_selection);
        void set_selection(core::Span<ecs::EntityId> selection);
        void clear_selection();

        static core::Span<std::pair<core::String, ecs::ComponentRuntimeInfo>> component_types();

    private:
};

}

#endif // EDITOR_EDITORWORLD_H

