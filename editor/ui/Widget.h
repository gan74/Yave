/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef EDITOR_UI_WIDGET_H
#define EDITOR_UI_WIDGET_H

#include <editor/editor.h>


#include <y/core/String.h>
#include <y/core/Vector.h>
#include <y/core/Span.h>

namespace editor {

class Widget : NonMovable {
    public:
        Widget(std::string_view title, u32 flags = 0);
        virtual ~Widget();

        virtual void refresh();

        const math::Vec2& position() const;
        const math::Vec2& size() const;

        bool is_focussed() const;
        bool is_mouse_inside() const;

        void set_parent(Widget* parent);
        bool has_parent() const;

        bool is_visible() const;

        void show();
        void close();

        std::string_view title() const;

        Y_TODO(remove this)
        void set_inside(Widget* parent);
        UiManager* manager() const;

        template<typename T, typename... Args>
        T* add_child(Args&&... args);

        virtual void paint(CmdBufferRecorder&) = 0;

    protected:
        friend class UiManager;

        virtual void before_paint() {}
        virtual void after_paint() {}

        virtual bool can_destroy() const;

        void refresh_all();
        void set_flags(u32 flags);
        void set_closable(bool closable);

        math::Vec2ui content_size() const;


        bool _visible = true;

    private:
        void update_attribs();

        core::String _title_with_id;
        std::string_view _title;

        math::Vec2 _position;
        math::Vec2 _size;
        math::Vec2 _min_size;

        u32 _flags = 0;

        bool _refresh_all = false;
        bool _closable = true;
        bool _docked = false;
        bool _focussed = false;
        bool _mouse_inside = false;

        void set_id(u64 id);
        void set_title(std::string_view title);

        u64 _id = 0;

        bool _has_children = false;
        Widget* _parent = nullptr;
        UiManager* _manager = nullptr;


};

}

#endif // EDITOR_UI_WIDGET_H

