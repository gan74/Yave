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
#ifndef EDITOR_PROPERTIES_COMPONENTWIDGETS_H
#define EDITOR_PROPERTIES_COMPONENTWIDGETS_H

#include <editor/editor.h>
#include <yave/ecs/ecs.h>

namespace editor {
namespace detail {
using component_widget_func_t = void (*)(ContextPtr ctx, ecs::EntityId id);
struct ComponentWidgetData {
    component_widget_func_t func = nullptr;
    ComponentWidgetData* next = nullptr;
};

void register_component_widget(ComponentWidgetData* data);
}

#define EDITOR_WIDGET_REG_RUNNER y_create_name_with_prefix(runner)
#define EDITOR_WIDGET_FUNC y_create_name_with_prefix(widget)

#define editor_widget_draw_func(...)                                                                    \
static void EDITOR_WIDGET_FUNC(ContextPtr ctx, ecs::EntityId id);                                       \
namespace {                                                                                             \
    class EDITOR_WIDGET_REG_RUNNER {                                                                    \
        EDITOR_WIDGET_REG_RUNNER() {                                                                    \
            comp_data.func = &EDITOR_WIDGET_FUNC;                                                       \
            detail::register_component_widget(&comp_data);                                              \
        }                                                                                               \
        static detail::ComponentWidgetData comp_data;                                                   \
        static EDITOR_WIDGET_REG_RUNNER runner;                                                         \
    };                                                                                                  \
    detail::ComponentWidgetData EDITOR_WIDGET_REG_RUNNER::comp_data = {};                               \
    EDITOR_WIDGET_REG_RUNNER EDITOR_WIDGET_REG_RUNNER::runner = {};                                     \
}                                                                                                       \
void EDITOR_WIDGET_FUNC(__VA_ARGS__)


void draw_component_widgets(ContextPtr ctx, ecs::EntityId id);

}


#endif // EDITOR_PROPERTIES_COMPONENTWIDGETS_H

