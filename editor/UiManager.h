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
#ifndef EDITOR_UIMANAGER_H
#define EDITOR_UIMANAGER_H

#include "Widget.h"

#include <y/core/Vector.h>
#include <y/core/FixedArray.h>
#include <y/core/Chrono.h>

#include <memory>

#include <functional>

namespace editor {

class UiManager : NonMovable {

    public:
        UiManager();
        ~UiManager();

        void on_gui();

        Widget* add_top_level_widget(std::unique_ptr<Widget> widget);
        
        core::Span<std::unique_ptr<Widget>> top_level_widgets() const;

        void close_all();

        Widget* last_focussed_widget();

        u32 generate_dock_id();
        u32 main_dock_id() const;

    private:
        friend class Widget;

        void update_fps_counter();
        void draw_fps_counter();
        void update_shortcuts();
        void draw_menu_bar();

        core::Vector<std::unique_ptr<Widget>> _widgets;

        Widget* _focussed = nullptr;
        Widget* _last_focussed = nullptr;

        core::Vector<const EditorAction*> _actions;
        core::Vector<std::pair<const EditorAction*, bool>> _shortcuts;

        core::FixedArray<char> _search_pattern = core::FixedArray<char>(256);

        core::StopWatch _timer;
        core::FixedArray<float> _frame_times = core::FixedArray<float>(60);
        float _total_time = 0.0f;
        u64 _frame_number = 0;

        u32 _dock_id = 0;
        u32 _main_dock_id = 0;
};

}

#endif // EDITOR_UIMANAGER_H

