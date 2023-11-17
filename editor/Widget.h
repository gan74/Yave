/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#include <tuple>

#define editor_widget_(type, on_startup, ...)                                                                       \
    inline static struct widget_register_t {                                                                        \
        widget_register_t() {                                                                                       \
            auto open_func = [] { editor::add_detached_widget<type>(); };                                           \
            static editor::EditorWidget widget = {                                                                  \
                #type, (on_startup), open_func, nullptr                                                             \
            };                                                                                                      \
            static constexpr usize arg_count = std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value;      \
            static const std::array<std::string_view, arg_count> menu = {__VA_ARGS__};                              \
            static editor::EditorAction action = {                                                                  \
                #type, "Open a new " #type, 0, yave::KeyCombination(), open_func, nullptr, menu, nullptr            \
            };                                                                                                      \
            editor::detail::register_widget(&widget);                                                               \
            editor::detail::register_action(&action);                                                               \
        }                                                                                                           \
        void trigger() {}                                                                                           \
    } widget_registerer;                                                                                            \



#define editor_widget(type, ...)        editor_widget_(type, false, __VA_ARGS__)
#define editor_widget_open(type, ...)   editor_widget_(type, true, __VA_ARGS__)


namespace editor {

struct EditorWidget {
    std::string_view name;
    bool open_on_startup = false;
    void (*create)() = nullptr;
    EditorWidget* next = nullptr;
};

const EditorWidget* all_widgets();

namespace detail {
void register_widget(EditorWidget* widget);
}



class Widget : NonMovable {

    public:
        Widget(std::string_view title, int flags = 0);
        virtual ~Widget();

        void close();
        bool is_visible() const;

        void set_visible(bool visible);

        void set_parent(Widget* parent);

        void set_modal(bool modal);

        virtual void refresh();
        virtual void refresh_all();

        void draw_gui_inside();

    protected:
        virtual void on_gui();
        virtual bool before_gui();
        virtual void after_gui();

        virtual bool should_keep_alive() const;

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
        bool _modal = false;

        Widget* _parent = nullptr;
        int _flags = 0;
};
}


#endif // EDITOR_WIDGET_H

