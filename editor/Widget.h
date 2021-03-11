/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include <editor/editor.h>

#include <y/core/String.h>

#include <y/utils/log.h>

#include <array>

// #include <y/serde3/poly.h>


#define editor_widget(type, ...)  editor_action_(#type, "Open a new " #type, 0, []{ editor::add_child_widget<type>(); }, __VA_ARGS__)

Y_TODO(change this?)
#define editor_widget_open(type, ...)  editor_action_(#type, "Open a new " #type, 0x01, []{ editor::add_child_widget<type>(); }, __VA_ARGS__)

namespace editor {

class Widget : NonMovable {

    public:
        Widget(std::string_view title, int flags = 0);
        virtual ~Widget();

        void close();
        bool is_visible() const;

        void set_visible(bool visible);

        void set_parent(Widget* parent);

        virtual void refresh();
        virtual void refresh_all();

        void draw_gui_inside();

        // y_reflect(_title_with_id, _title, _id)
        // y_serde3_poly_base(Widget)

    protected:
        virtual void on_gui();
        virtual bool before_gui();
        virtual void after_gui();

        math::Vec2ui content_size() const;

        void set_flags(int flags);

    private:
        friend class UiManager;

        void draw(bool inside);

        void set_id(u64 id);
        void set_title(std::string_view title);

        core::String _title_with_id;

        std::string_view _title;
        u64 _id = 0;

        bool _visible = true;

        Widget* _parent = nullptr;
        int _flags = 0;
};
}


#endif // EDITOR_WIDGET_H

