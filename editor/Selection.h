/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#ifndef EDITOR_SELECTION_H
#define EDITOR_SELECTION_H

#include <editor/editor.h>

#include <yave/ecs/ecs.h>

#include <y/core/HashMap.h>

namespace editor {

class Selection {
    public:
        usize selected_entities_count() const {
            return _ids.size();
        }

        auto selected_entities() const {
            return _ids.keys();
        }

        ecs::EntityId selected_entity() const;

        bool is_selected(ecs::EntityId id) const;

        void add_or_remove(ecs::EntityId id, bool set = false);

        void set_selected(ecs::EntityId id);
        void set_selected(core::Span<ecs::EntityId> id);

        void clear_selection();


    private:
        core::FlatHashMap<ecs::EntityId, int> _ids;
};

}

#endif // EDITOR_CONTEXT_SELECTION_H

