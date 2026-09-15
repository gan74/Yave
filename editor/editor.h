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

#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H


#include <yave/utils/forward.h>
#include <editor/utils/forward.h>

#include "Settings.h"

#include <y/core/Span.h>
#include <y/concurrent/JobSystem.h>
#include <y/utils/traits.h>

#include <memory>
#include <string_view>
#include <type_traits>
#include <concepts>


// Just for convenience
#include <editor/utils/renderdochelper.h>


namespace editor {

using namespace yave;

void init_editor(ImGuiPlatform* platform, const Settings& settings = Settings());
void destroy_editor();

void run_editor();

Settings& app_settings();
UiManager& ui();
AssetStore& asset_store();
AssetLoader& asset_loader();
ThumbnailRenderer& thumbnail_renderer();
concurrent::JobSystem& editor_job_system();

const EditorResources& resources();


DebugValues& debug_values();
DirectDraw& debug_drawer();


Workspace* current_workspace();
void set_current_workspace(Workspace* workspace);
void unset_current_workspace(Workspace* workspace);


Widget* last_focussed_widget();

template<typename T>
T* last_focussed_widget_typed() {
    return dynamic_cast<T*>(last_focussed_widget());
}

Widget* add_top_level_widget(std::unique_ptr<Widget> widget);

template<typename T, typename... Args>
T* add_top_level_widget(Args&&... args) {
    return dynamic_cast<T*>(add_top_level_widget(std::make_unique<T>(y_fwd(args)...)));
}




using UiTexture = void*;



struct EditorAction {
    enum Flags : u32 {
        None            = 0x00,
        Contextual      = 0x01,
        Widget          = 0x02,
    };

    std::string_view name;
    std::string_view description;
    Flags flags = Flags::None;
    KeyCombination shortcut;
    void (*function)(Workspace*) = nullptr;
    bool (*enabled)(Workspace*) = nullptr;
    core::Span<std::string_view> menu;
    EditorAction const* next = nullptr;
};

const EditorAction* all_actions();

namespace detail {
void register_action(EditorAction* action);

template<typename F>
bool action_matches_workspace(F&&, Workspace* workspace) {
    if constexpr(std::same_as<std::remove_cvref_t<F>, nullptr_t> || std::invocable<F>) {
        return true;
    } else if constexpr(std::invocable<F, Workspace*>) {
        return !!workspace;
    } else {
        using WS = std::remove_pointer_t<typename function_traits<F>::template arg_type<0>>;
        return dynamic_cast<WS*>(workspace);
    }
}

template<typename F>
bool invoke_action_func(F&& func, Workspace* workspace) {
    if constexpr(!std::same_as<std::remove_cvref_t<F>, nullptr_t>) {
        using traits = function_traits<F>;
        auto invoke_as_bool = [&](auto&&... args) -> bool {
            if constexpr(std::same_as<typename traits::return_type, bool>) {
                return func(y_fwd(args)...);
            } else {
                func(y_fwd(args)...);
                return true;
            }
        };

        if constexpr(std::invocable<F>) {
            return invoke_as_bool();
        } else if constexpr(std::invocable<F, Workspace*>) {
            return workspace && invoke_as_bool(workspace);
        } else {
            using WS = std::remove_pointer_t<typename traits::template arg_type<0>>;
            if(WS* w = dynamic_cast<WS*>(workspace)) {
                return invoke_as_bool(w);
            }
            return false;
        }
    }
    return true;
}
}

}


#define editor_action_(name, desc, flags, shortcut, func, enabled, ...)                                         \
    namespace {                                                                                                 \
        inline static struct y_create_name_with_prefix(action_register_t) {                                     \
            y_create_name_with_prefix(action_register_t)() {                                                    \
                static constexpr std::string_view names[] = { name, __VA_ARGS__ };                              \
                static editor::EditorAction action = {                                                          \
                    names[0], desc, (flags), yave::KeyCombination(shortcut),                                    \
                    [](editor::Workspace* w) { editor::detail::invoke_action_func(func, w); },                  \
                    [](editor::Workspace* w) {                                                                  \
                        return editor::detail::action_matches_workspace(func, w)                                \
                            && editor::detail::invoke_action_func(enabled, w);                                  \
                    },                                                                                          \
                    y::core::Span<std::string_view>(names + 1, std::size(names) - 1), nullptr                   \
                };                                                                                              \
                editor::detail::register_action(&action);                                                       \
            }                                                                                                   \
            void trigger() {}                                                                                   \
        } y_create_name_with_prefix(action_register);                                                           \
    }                                                                                                           \
    void y_register_action(y_create_name_with_prefix(action_register_t)) {                                      \
        y_create_name_with_prefix(action_register).trigger();                                                   \
    }


#define editor_action_shortcut(name, shortcut, func, ...)   editor_action_(name, "", EditorAction::None, shortcut, func, nullptr, __VA_ARGS__)
#define editor_action_desc(name, desc, func, ...)           editor_action_(name, desc, EditorAction::None, /* no shortcut */, func, nullptr, __VA_ARGS__)
#define editor_action_contextual(name, func, enabled, ...)  editor_action_(name, "", EditorAction::Contextual, /* no shortcut */, func, enabled, __VA_ARGS__)
#define editor_action_enable(name, func, enabled, ...)      editor_action_(name, "", EditorAction::None, /* no shortcut */, func, enabled, __VA_ARGS__)
#define editor_action(name, func, ...)                      editor_action_desc(name, "", func, __VA_ARGS__)


#endif // EDITOR_EDITOR_H
