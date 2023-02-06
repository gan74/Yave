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

#include <editor/editor.h>
#include <editor/EditorWorld.h>

#include <editor/UiManager.h>

#include <y/utils/log.h>

#include <external/imgui_test_engine/imgui_te_context.h>

#include <iterator>

namespace editor {

static core::Vector<core::String> window_names() {
    core::Vector<core::String> names;
    for(const ImGuiWindow* w : ImGui::GetCurrentContext()->Windows) {
        if(w->Hidden || !w->WasActive) {
            continue;
        }
        names.emplace_back(w->Name);
    }
    return names;
}

static usize entity_count(const EditorWorld& world = current_world()) {
    return world.entity_count();
}

static core::Vector<ecs::EntityId> all_ids(const EditorWorld& world = current_world()) {
    core::Vector<ecs::EntityId> entities;
    for(const ecs::EntityId id : current_world().ids()) {
        entities << id;
    }
    return entities;
}


// https://github.com/ocornut/imgui_test_engine/wiki/Named-References
void register_editor_tests(ImGuiTestEngine* engine) {
    log_msg("Registering ImGui tests", Log::Debug);

    IM_REGISTER_TEST(engine, "tests", "new scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("File/New");
        y_debug_assert(!entity_count());
    };

#if 0
    IM_REGISTER_TEST(engine, "tests", "open component panel")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->ItemClick("**/" ICON_FA_SEARCH "##searchbar");
        ctx->KeyChars("comp");
        ctx->KeyPress(ImGuiKey_DownArrow);
        ctx->KeyPress(ImGuiKey_Enter);
        ctx->SetRef(ICON_FA_WRENCH " Components##1");
    };
#endif

    IM_REGISTER_TEST(engine, "tests", "close all")->TestFunc = [](ImGuiTestContext* ctx) {
        for(const auto& name : window_names()) {
            if(ImGuiWindow* window = ctx->GetWindowByRef(name.data())) {
                if(window->HasCloseButton) {
                    ctx->UndockWindow(name.data());
                    ctx->WindowClose(name.data());
                }
            }
        }
    };

    IM_REGISTER_TEST(engine, "tests", "restore default layout")->TestFunc = [=](ImGuiTestContext* ctx) {
        ctx->ItemClick("**/" ICON_FA_SEARCH "##searchbar");
        ctx->KeyChars("restore d");

        ImGuiTestItemList items;
        ctx->GatherItems(&items, "##suggestionpopup");
        IM_CHECK_EQ(items.size(), 1);

        ctx->MouseMoveToPos(items[0]->RectClipped.GetCenter());
        ctx->MouseClick();
    };

    IM_REGISTER_TEST(engine, "tests", "add empty entities")->TestFunc = [=](ImGuiTestContext* ctx) {
        ctx->SetRef(ICON_FA_CUBES " Entities##1");
        for(usize i = 0; i != 4; ++i) {
            ctx->ItemClick(ICON_FA_PLUS);
            ctx->ItemClick("**/" ICON_FA_PLUS " New empty entity");
        }
    };

    IM_REGISTER_TEST(engine, "tests", "rename entity")->TestFunc = [=](ImGuiTestContext* ctx) {
        const auto ids = all_ids();
        y_debug_assert(!ids.is_empty());

        ctx->ItemClick(fmt_c_str("//" ICON_FA_CUBES " Entities##1/**/###%", ids[ids.size() / 2].as_u64()), ImGuiMouseButton_Right);
        ctx->ItemClick("**/Rename");

        ctx->SetRef("//Rename##1");
        ctx->ItemInput("##name");
        ctx->KeyChars("flubudu");
        ctx->ItemClick("Ok");

        const core::String label = ctx->ItemInfo(fmt_c_str("//" ICON_FA_CUBES " Entities##1/**/###%", ids[ids.size() / 2].as_u64()))->DebugLabel;
        y_debug_assert(label.starts_with(ICON_FA_DATABASE " flubudu"));
    };

    IM_REGISTER_TEST(engine, "tests", "remove entity")->TestFunc = [=](ImGuiTestContext* ctx) {
        const auto ids = all_ids();
        y_debug_assert(!ids.is_empty());

        {
            ctx->SetRef("//" ICON_FA_CUBES " Entities##1");
            ctx->ItemClick(fmt_c_str("**/###%", ids[ids.size() / 3].as_u64()), ImGuiMouseButton_Right);
            ctx->ItemClick("**/" ICON_FA_TRASH " Delete");
            ctx->ItemClick("//Confirm##1/Cancel");
        }

        ctx->Yield();

        {
            ctx->SetRef("//" ICON_FA_CUBES " Entities##1");
            ctx->ItemClick(fmt_c_str("**/###%", ids[ids.size() / 3].as_u64()), ImGuiMouseButton_Right);
            ctx->ItemClick("**/" ICON_FA_TRASH " Delete");
            ctx->ItemClick("//Confirm##1/Ok");
        }

        y_debug_assert(entity_count() == ids.size() - 1);
    };

    IM_REGISTER_TEST(engine, "tests", "save scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("File/" ICON_FA_SAVE " Save");
    };

    IM_REGISTER_TEST(engine, "tests", "load scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");

        ctx->MenuClick("File/New");
        y_debug_assert(!entity_count());

        ctx->MenuClick("File/" ICON_FA_FOLDER " Load");
        y_debug_assert(entity_count() == 3);
    };
}

}
