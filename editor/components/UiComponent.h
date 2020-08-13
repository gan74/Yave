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
#ifndef EDITOR_COMPONENTS_UICOMPONENT_H
#define EDITOR_COMPONENTS_UICOMPONENT_H

#include <editor/editor.h>

#include <editor/ui/UiElement.h>

#include <yave/ecs/ecs.h>

#include <y/serde3/poly.h>
#include <y/serde3/archives.h>

namespace editor {

struct UiWidgetBase : NonMovable {

    public:
        UiWidgetBase(std::string_view title, u32 = 0);

        virtual ~UiWidgetBase();

        virtual void paint(ecs::EntityWorld&, CmdBufferRecorder&);

         // ------------------------------- COMPAT -------------------------------
        virtual void paint_ui(CmdBufferRecorder&, const FrameToken&);

        virtual void before_paint() {}
        virtual void after_paint() {}
        virtual void refresh() {}


        void set_title(std::string_view title);
        void close();
        void show();

        const math::Vec2& position() const;
        const math::Vec2& size() const;
        math::Vec2ui content_size() const;

        bool is_focussed() const;
        bool is_mouse_inside() const;

        y_serde3_poly_abstract_base(UiWidgetBase)
        y_reflect(_title_with_id, _title_size)

    private:
        friend class UiSystem;

        void update_attribs();

        ecs::EntityId _entity_id;

        core::String _title_with_id;
        usize _title_size;

        math::Vec2 _position;
        math::Vec2 _size;
        math::Vec2 _content_size;

        bool _mouse_inside = false;
        bool _docked = false;
        bool _focussed = false;
        bool _visible = true;

};

template<typename Derived>
struct UiWidget : UiWidgetBase {
    using UiWidgetBase::UiWidgetBase;
    y_serde3_poly(Derived)
};

struct UiComponent {
    static ecs::EntityId create_widget(ecs::EntityWorld& world, std::unique_ptr<UiWidgetBase> wid);

    template<typename F>
    static ecs::EntityId create_widget(ecs::EntityWorld& world, std::string_view title, F&& f) {
        struct Wid : UiWidget<Wid> {
            Wid(std::string_view title, F&& f) : UiWidget<Wid>(title), _func(std::move(f)) {
            }

            void paint(ecs::EntityWorld&, CmdBufferRecorder&) override {
                _func();
            }

            F _func;
        };
        return create_widget(world, std::make_unique<Wid>(title, y_fwd(f)));
    }

    std::unique_ptr<UiWidgetBase> widget;
    ecs::EntityId parent;

    Y_TODO(Fix non serializable components breaking everything)
    y_reflect(parent)
};

}

#endif // EDITOR_COMPONENTS_UICOMPONENT_H

