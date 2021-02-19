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
#ifndef EDITOR_CONTEXT_UIMANAGER_H
#define EDITOR_CONTEXT_UIMANAGER_H

#include <editor/ui/Widget.h>

#include <y/core/Chrono.h>
#include <y/core/HashMap.h>

#include <y/utils/iter.h>

#include <typeindex>

namespace editor {

class ImGuiRenderer;

class UiManager : NonMovable, public ContextLinked {

    struct Ids {
        core::Vector<u64> released;
        u64 next = 0;
    };

    public:
        UiManager(ContextPtr ctx);
        ~UiManager();

        core::Span<std::unique_ptr<Widget>> widgets() const;

        const ImGuiRenderer& renderer() const;

        void refresh_all();

        // don't spam these two: they are synchronous and modal (and not supported outside of win32 right now)
        bool confirm(const char* message);
        void ok(const char* title, const char* message);

        void paint(CmdBufferRecorder& recorder, const FrameToken& token);

        template<typename T>
        T* find() {
            for(auto&& e : _widgets) {
                if(T* w = dynamic_cast<T*>(e.get())) {
                    return w;
                }
            }
            return nullptr;
        }

        template<typename T>
        T* show() {
            if(T* w = find<T>()) {
                w->show();
                return w;
            }
            return add_widget<T>();
        }

        template<typename T, typename... Args>
        T* add_widget(Args&&... args) {
            if constexpr(std::is_constructible_v<T, ContextPtr, Args...>) {
                _widgets.emplace_back(std::make_unique<T>(context(), y_fwd(args)...));
            } else {
                _widgets.emplace_back(std::make_unique<T>(y_fwd(args)...));
            }
            Widget* ptr = _widgets.last().get();
            ptr->_manager = this;
            set_id(ptr);
            return dynamic_cast<T*>(ptr);
        }

    private:
        static void paint_widget(Widget* widget, CmdBufferRecorder& recorder);
        static bool paint(core::Span<std::unique_ptr<Widget>> elements, CmdBufferRecorder& recorder);

        void paint_menu_bar();
        void paint_widgets(CmdBufferRecorder& recorder);

        void cull_closed();

        Ids& ids_for(Widget* elem);
        void set_id(Widget* elem);


        core::Vector<std::unique_ptr<Widget>> _widgets;
        core::ExternalHashMap<std::type_index, Ids> _ids;

        std::unique_ptr<ImGuiRenderer> _renderer;

        core::Chrono _frame_timer;

};

template<typename T, typename... Args>
T* Widget::add_child(Args&&... args) {
    T* wid = manager()->add_widget<T>(y_fwd(args)...);
    wid->set_parent(this);
    return wid;
}

}

#endif // EDITOR_CONTEXT_UIMANAGER_H

