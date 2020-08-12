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

        core::Span<std::unique_ptr<UiElement>> ui_elements() const;

        const ImGuiRenderer& renderer() const;

        void refresh_all();

        // don't spam these two: they are synchronous and modal (and not supported outside of win32 right now)
        bool confirm(const char* message);
        void ok(const char* title, const char* message);


        void paint(CmdBufferRecorder& recorder, const FrameToken& token);

        template<typename T>
        T* find() {
            for(auto&& e : _elements) {
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
            return add<T>();
        }

        template<typename T, typename... Args>
        T* add(Args&&... args) {
            if constexpr(std::is_constructible_v<T, ContextPtr, Args...>) {
                _elements.emplace_back(std::make_unique<T>(context(), y_fwd(args)...));
            } else {
                _elements.emplace_back(std::make_unique<T>(y_fwd(args)...));
            }
            UiElement* ptr = _elements.last().get();
            set_id(ptr);
            return dynamic_cast<T*>(ptr);
        }

    private:
        static bool paint(core::Span<std::unique_ptr<UiElement>> elements, CmdBufferRecorder& recorder, const FrameToken& token);
        static void refresh_all(core::Span<std::unique_ptr<UiElement>> elements);

        void cull_closed(core::Vector<std::unique_ptr<UiElement>>& elements, bool is_child = false);
        Ids& ids_for(UiElement* elem);
        void set_id(UiElement* elem);

        void paint_ui(CmdBufferRecorder& recorder, const FrameToken& token);

        core::Vector<std::unique_ptr<UiElement>> _elements;
        core::ExternalHashMap<std::type_index, Ids> _ids;

        std::unique_ptr<ImGuiRenderer> _renderer;

        core::Chrono _frame_timer;

};

}

#endif // EDITOR_CONTEXT_UIMANAGER_H

