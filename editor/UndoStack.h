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

namespace editor {

class UndoStack : NonMovable {
    public:
        struct Item {
            core::String name;
            ecs::EntityId id;
            std::function<void(EditorWorld&, ecs::EntityId)> undo;
            std::function<void(EditorWorld&, ecs::EntityId)> redo;
        };

        UndoStack();

        void push(Item item);

        void undo();
        void redo();

        core::Span<Item> items() const;
        usize stack_top() const;

    private:
        friend class UndoStackWidget;

        core::Vector<Item> _items;
        usize _top = 0;
};

}

#endif // EDITOR_UNDOSTACK_H

