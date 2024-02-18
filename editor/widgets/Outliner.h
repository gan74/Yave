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
#ifndef EDITOR_WIDGETS_OUTLINER_H
#define EDITOR_WIDGETS_OUTLINER_H

#include <yave/ecs/EntityWorld.h>

#include <editor/Widget.h>

namespace editor {

class Outliner final : public Widget {

    editor_widget_open(Outliner)

    public:
        Outliner();

    protected:
        void on_gui() override;

    private:
        void display_node(EditorWorld& world, ecs::EntityId id);
        bool make_drop_target(EditorWorld& world, ecs::EntityId id);

        core::Vector<std::tuple<const char*, core::String, bool>> _tag_buttons;

        ecs::EntityId _context_menu_target;
        ecs::EntityId _click_target;
};

}

#endif // EDITOR_WIDGETS_OUTLINER_H

