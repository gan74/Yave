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
#ifndef EDITOR_WIDGETS_COMPONENTPANEL_H
#define EDITOR_WIDGETS_COMPONENTPANEL_H

#include <yave/ecs/EntityWorld.h>

#include <editor/Widget.h>

namespace editor {

class ComponentPanelWidgetBase;

class ComponentPanel final : public Widget {

    editor_widget_open(ComponentPanel)

    public:
        ComponentPanel();

    protected:
        void on_gui() override;

    private:
        core::Vector<std::unique_ptr<ComponentPanelWidgetBase>> _widgets;
};



class ComponentPanelWidgetBase : NonMovable {
    public:
        virtual ~ComponentPanelWidgetBase() = default;

        virtual void process_entity(ecs::EntityWorld& world, ecs::EntityId id) = 0;
        virtual ecs::ComponentRuntimeInfo runtime_info() const = 0;

    protected:
        friend class ComponentPanel;

        struct Link {
            Link* next = nullptr;
            std::unique_ptr<ComponentPanelWidgetBase> (*create)() = nullptr;
        };

        static Link* _first_link;
        static void register_link(Link* link);

};

template<typename CRTP, typename T>
class ComponentPanelWidget : public ComponentPanelWidgetBase {

    public:
        ComponentPanelWidget() {
            _registerer.trigger();
        }

        void process_entity(ecs::EntityWorld& world, ecs::EntityId id) override {
            if(T* component = world.component<T>(id)) {
                static_cast<CRTP*>(this)->on_gui(id, component);
            }
        }

        ecs::ComponentRuntimeInfo runtime_info() const override {
            return ecs::ComponentRuntimeInfo::create<T>();
        }

    private:
        static inline struct Registerer {
            Registerer() {
                static Link link = {
                    nullptr,
                    []() -> std::unique_ptr<ComponentPanelWidgetBase> { return std::make_unique<CRTP>(); }
                };
                register_link(&link);
            }
            void trigger() {};
        } _registerer;
};

}

#endif // EDITOR_WIDGETS_COMPONENTPANEL_H

