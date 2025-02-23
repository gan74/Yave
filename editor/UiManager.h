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
#ifndef EDITOR_UIMANAGER_H
#define EDITOR_UIMANAGER_H

#include "Widget.h"

#include <y/core/Vector.h>
#include <y/core/HashMap.h>
#include <y/core/FixedArray.h>
#include <y/core/Chrono.h>

#include <typeindex>
#include <memory>

#include <functional>

namespace editor {

class UiManager : NonMovable {

    struct WidgetIdStack {
        core::Vector<u64> released;
        u64 next = 0;
    };

    public:
        UiManager();
        ~UiManager();

        void on_gui();

        Widget* add_widget(std::unique_ptr<Widget> widget, bool auto_parent = true);

        void restore_default_layout();
        void open_default_widgets();
        void close_all();

        core::Span<std::unique_ptr<Widget>> widgets() const;

    private:
        void update_fps_counter();
        void draw_fps_counter();
        void update_shortcuts();
        void draw_menu_bar();
        void set_widget_id(Widget* widget);

        core::Vector<std::unique_ptr<Widget>> _widgets;
        core::FlatHashMap<std::type_index, WidgetIdStack> _ids;

        Widget* _auto_parent = nullptr;

        core::Vector<const EditorAction*> _actions;
        core::Vector<std::pair<const EditorAction*, bool>> _shortcuts;

        core::FixedArray<char> _search_pattern = core::FixedArray<char>(256);

        core::StopWatch _timer;
        core::FixedArray<float> _frame_times = core::FixedArray<float>(60);
        float _total_time = 0.0f;
        u64 _frame_number = 0;
};

}

#endif // EDITOR_UIMANAGER_H

