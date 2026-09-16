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
#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include <editor/editor.h>

#include <y/core/String.h>

#include <y/utils/log.h>

#include <external/imgui/imgui.h>

#include <array>
#include <memory>
#include <tuple>
#include <type_traits>


namespace editor {


class Widget : NonMovable {

    public:
        Widget(std::string_view title, int flags = 0);
        virtual ~Widget();

        void close();

        bool is_visible() const;

        void set_visible(bool visible);

        void set_modal(bool modal);
        bool is_modal() const;

        u64 widget_id() const;

        virtual void refresh();
        virtual void refresh_all();

        virtual bool belongs_to(const Workspace*) const; 

        void draw_gui_inside();

    protected:
        virtual void on_gui();
        virtual void on_inactive_gui();
        virtual bool before_gui();
        virtual void after_gui();

        virtual bool should_keep_alive() const;

        bool is_focussed() const;

        math::Vec2ui content_size() const;

        ImGuiWindowClass _window_class = {};

    private:
        friend class UiManager;

        void draw(bool inside);

        void set_title(std::string_view title);

        core::String _title_with_id;
        const u64 _id;

        bool _visible = true;
        bool _modal = false;
        bool _focussed = false;

        int _flags = 0;
};


class WorkspaceWidgetBase : public Widget {
    protected:
        using Widget::Widget;
};


template<typename W>
class WorkspaceWidget : public WorkspaceWidgetBase {
    public:
        using workspace_type = W;

        WorkspaceWidget(std::string_view title, W* workspace, int flags = 0) :
                WorkspaceWidgetBase(title, flags),
                _workspace(workspace) {

            y_debug_assert(_workspace);

            _window_class.ClassId = _workspace->workspace_id();
        }

        W* workspace() const {
            return _workspace;
        }

        bool belongs_to(const Workspace* workspace) const override {
            return workspace == _workspace;
        }

    protected:
        void after_gui() override {
            if(is_focussed()) {
                set_current_workspace(_workspace);
            }
            Widget::after_gui();
        }

        W* _workspace = nullptr;
};





struct EditorWidget {
    std::string_view name;
    bool open_on_startup = false;
    std::unique_ptr<Widget> (*create)(Workspace*) = nullptr;
    EditorWidget* next = nullptr;
};

const EditorWidget* all_widgets();

namespace detail {
void register_widget(EditorWidget* widget);

template<typename T>
std::unique_ptr<Widget> create_workspace_widget(Workspace* workspace) {
    if constexpr(std::is_base_of_v<WorkspaceWidgetBase, T>) {
        if(auto* ws = dynamic_cast<typename T::workspace_type*>(workspace)) {
            return std::make_unique<T>(ws);
        }
    } else {
        return std::make_unique<T>();
    }
    return nullptr;
}

template<typename T>
bool can_create_workspace_widget(Workspace* workspace) {
    if constexpr(std::is_base_of_v<WorkspaceWidgetBase, T>) {
        return dynamic_cast<typename T::workspace_type*>(workspace);
    }
    return true;
}

}

}


#define editor_widget_(type, on_startup, ...)                                                                               \
        inline static struct widget_register_t {                                                                            \
            widget_register_t() {                                                                                           \
            static editor::EditorWidget widget = {                                                                          \
                #type, (on_startup), editor::detail::create_workspace_widget<type>, nullptr                                 \
            };                                                                                                              \
            static constexpr usize arg_count = std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value;              \
            static const std::array<std::string_view, arg_count> menu = {__VA_ARGS__};                                      \
            static editor::EditorAction action = {                                                                          \
                #type, "Open a new " #type, EditorAction::Widget, yave::KeyCombination(),                                   \
                [](Workspace* w) { editor::add_top_level_widget(editor::detail::create_workspace_widget<type>(w)); },       \
                editor::detail::can_create_workspace_widget<type>,                                                          \
                menu, nullptr                                                                                               \
            };                                                                                                              \
            editor::detail::register_widget(&widget);                                                                       \
            editor::detail::register_action(&action);                                                                       \
        }                                                                                                                   \
        void trigger() {}                                                                                                   \
    } widget_registerer;

#define editor_widget(type, ...)        editor_widget_(type, false, __VA_ARGS__)
#define editor_widget_open(type, ...)   editor_widget_(type, true, __VA_ARGS__)




#endif // EDITOR_WIDGET_H
