/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

#include <yave/window/EventHandler.h>

#include <yave/utils/forward.h>
#include <editor/utils/forward.h>

#include <y/core/Span.h>

#include <memory>
#include <string_view>


// Just for convenience
#include <editor/utils/renderdochelper.h>


namespace editor {

using namespace yave;


EditorApplication* application();

Settings& app_settings();

AssetStore& asset_store();
AssetLoader& asset_loader();
ThumbmailRenderer& thumbmail_renderer();

EditorWorld& current_world();
const SceneView& scene_view();

DirectDraw& debug_drawer();

PendingOpsQueue& pending_ops_queue();

const EditorResources& resources();
UiManager& ui();


Widget* add_widget(std::unique_ptr<Widget> widget, bool auto_parent = true);

template<typename T, typename... Args>
T* add_child_widget(Args&&... args) {
    return dynamic_cast<T*>(add_widget(std::make_unique<T>(y_fwd(args)...), true));
}

template<typename T, typename... Args>
T* add_detached_widget(Args&&... args) {
    return dynamic_cast<T*>(add_widget(std::make_unique<T>(y_fwd(args)...), false));
}




struct EditorAction {
    std::string_view name;
    std::string_view description;
    u32 flags = 0;
    KeyCombination shortcut;
    void (*function)() = nullptr;
    core::Span<std::string_view> menu;
    EditorAction const* next = nullptr;
};

const EditorAction* all_actions();

namespace detail {
void register_action(EditorAction* action);
}

}


#define editor_action_(name, desc, flags, shortcut, func, ...)                                          \
    struct y_create_name_with_prefix(action_trigger_t) {                                                \
        inline static struct action_register_t {                                                        \
            action_register_t() {                                                                       \
                static constexpr std::string_view names[] = { name, __VA_ARGS__ };                      \
                static editor::EditorAction action = {                                                  \
                    names[0], desc, (flags), yave::KeyCombination(shortcut), []{ func(); },             \
                    y::core::Span<std::string_view>(names + 1, std::size(names) - 1), nullptr           \
                };                                                                                      \
                editor::detail::register_action(&action);                                               \
            }                                                                                           \
            void trigger() {}                                                                           \
        } action_registerer;                                                                            \
        void trigger() { action_registerer.trigger(); }                                                 \
    };


#define editor_action_shortcut(name, shortcut, func, ...) editor_action_(name, "", 0, shortcut, func, __VA_ARGS__)
#define editor_action_desc(name, desc, func, ...) editor_action_(name, desc, 0, /* no shortcut */, func, __VA_ARGS__)
#define editor_action(name, func, ...) editor_action_desc(name, "", func, __VA_ARGS__)


#endif // EDITOR_EDITOR_H

