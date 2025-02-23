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
#ifndef EDITOR_UNDOSTACK_H
#define EDITOR_UNDOSTACK_H

#include <editor/editor.h>

#include <yave/ecs/ecs.h>

#include <y/io2/Buffer.h>
#include <y/core/Chrono.h>

#include <source_location>

namespace editor {

class UndoStack : NonMovable {
    struct UndoId {
        u32 id = 0;
    };

    public:
        using UndoFunc = std::function<void(EditorWorld&)>;

        struct Item {
            core::String name;
            UndoFunc undo;
            UndoFunc redo;
            bool merge_with_prev;
        };

        static constexpr auto merge_time = core::Duration::seconds(0.2);

        UndoStack();

        void clear();

        void push(core::String name, UndoFunc undo, UndoFunc redo, UndoId id, bool redo_immediately = false);

        void undo(bool merged = true);
        void redo(bool merged = true);

        core::Span<Item> items() const;
        usize stack_top() const;

        static UndoId generate_static_id();

    private:

        core::Vector<Item> _items;
        usize _top = 0;
        UndoId _last_id;
        core::Chrono _since_last;
};

}

#endif // EDITOR_UNDOSTACK_H

