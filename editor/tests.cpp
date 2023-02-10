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

#if 0
    IM_REGISTER_TEST(engine, "tests", "new scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("File/New");
        IM_CHECK_EQ(entity_count(), 0);
    };

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
        IM_CHECK_NE(ids.size(), 0);
        const ecs::EntityId id = ids[ids.size() / 2];

        ctx->ItemClick(fmt_c_str("//" ICON_FA_CUBES " Entities##1/**/###%", id.as_u64()), ImGuiMouseButton_Right);
        ctx->ItemClick("//$FOCUSED/Rename");

        ctx->SetRef("//Rename##1");
        ctx->ItemInput("##name");
        ctx->KeyChars("flubudu");
        ctx->ItemClick("Ok");

        const core::String label = ctx->ItemInfo(fmt_c_str("//" ICON_FA_CUBES " Entities##1/**/###%", id.as_u64()))->DebugLabel;
        IM_CHECK_EQ(label.starts_with(ICON_FA_DATABASE " flubudu"), true);
    };

    IM_REGISTER_TEST(engine, "tests", "remove entity")->TestFunc = [=](ImGuiTestContext* ctx) {
        const auto ids = all_ids();
        IM_CHECK_NE(ids.size(), 0);
        const ecs::EntityId id = ids[ids.size() / 3];

        {
            ctx->SetRef("//" ICON_FA_CUBES " Entities##1");
            ctx->ItemClick(fmt_c_str("**/###%", id.as_u64()), ImGuiMouseButton_Right);
            ctx->ItemClick("//$FOCUSED/" ICON_FA_TRASH " Delete");
            ctx->ItemClick("//$FOCUSED/Cancel");
        }

        ctx->Yield();

        {
            ctx->SetRef("//" ICON_FA_CUBES " Entities##1");
            ctx->ItemClick(fmt_c_str("**/###%", id.as_u64()), ImGuiMouseButton_Right);
            ctx->ItemClick("//$FOCUSED/" ICON_FA_TRASH " Delete");
            ctx->ItemClick("//$FOCUSED/Ok");
        }

        IM_CHECK_EQ(entity_count(), ids.size() - 1);
    };

    IM_REGISTER_TEST(engine, "tests", "save scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("File/" ICON_FA_SAVE " Save");
    };

    IM_REGISTER_TEST(engine, "tests", "load scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");

        ctx->MenuClick("File/New");
        IM_CHECK_EQ(entity_count(), 0);

        ctx->MenuClick("File/" ICON_FA_FOLDER " Load");
        IM_CHECK_EQ(entity_count(), 3);
    };
#endif
    IM_REGISTER_TEST(engine, "tests", "clear resources")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("View/ResourceBrowser");

        ctx->Yield();

        ctx->SetRef("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");

        ImGuiTestItemList items;
        ctx->GatherItems(&items, "");
        log_msg(fmt("% items", items.size()));
        for(const ImGuiTestItemInfo& info : items) {
            log_msg(fmt("  %: '%', table: %", info.ID, info.DebugLabel, !!ImGui::TableFindByID(info.ID)));
        }
        //const ImGuiTestItemInfo* info = ctx->ItemInfoOpenFullPath("**/##filetable");
        //IM_CHECK_NE(info->ID, 0);


        ctx->WindowClose("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");
    };

    IM_REGISTER_TEST(engine, "tests", "create resource folder")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("View/ResourceBrowser");

        ctx->Yield();

        ctx->SetRef("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");
        const ImGuiTestItemInfo* info = ctx->ItemInfoOpenFullPath("");
        IM_CHECK_NE(info->ID, 0);
        ctx->MouseMoveToPos(info->RectClipped.GetCenter());
        ctx->MouseClick(ImGuiMouseButton_Right);
        ctx->ItemClick("**/New folder");

        ctx->WindowClose("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");
    };

#if 0
    IM_REGISTER_TEST(engine, "tests", "add static mesh")->TestFunc = [](ImGuiTestContext* ctx) {
        const auto ids = all_ids();
        IM_CHECK_NE(ids.size(), 0);
        const ecs::EntityId id = ids[0];

        {
            ctx->SetRef("//" ICON_FA_CUBES " Entities##1");
            ctx->ItemClick(fmt_c_str("**/###%", id.as_u64()));
        }

        ctx->Yield();

        {
            ctx->SetRef("//" ICON_FA_WRENCH " Components##1");

            {
                ctx->ItemOpen("**/" ICON_FA_PUZZLE_PIECE " Entity");
                ctx->ItemInputValue("**/##name", "shalg");
                ctx->ItemClose("**/" ICON_FA_PUZZLE_PIECE " Entity");
            }

            ctx->ItemClick("**/" ICON_FA_PLUS " Add component");
            ctx->ItemClick("//$FOCUSED/" ICON_FA_PUZZLE_PIECE " StaticMeshComponent");

            {
                ctx->ItemOpen("**/" ICON_FA_PUZZLE_PIECE " TransformableComponent");
                ctx->ItemInputValue("**/##position/##x", 1.0f);
                ctx->ItemInputValue("**/##position/##z", 2.0f);
                ctx->ItemClose("**/" ICON_FA_PUZZLE_PIECE " TransformableComponent");
            }

            {
                ctx->ItemOpen("**/" ICON_FA_PUZZLE_PIECE " StaticMeshComponent");
                IM_CHECK_NE(ctx->ItemInfoOpenFullPath("**/" ICON_FA_FOLDER_OPEN)->ID, 0);
                ctx->ItemClose("**/" ICON_FA_PUZZLE_PIECE " StaticMeshComponent");
            }
        }

        ctx->Yield();

        {
            ctx->SetRef("//" ICON_FA_CUBES " Entities##1");
            const ImGuiTestItemInfo* info = ctx->ItemInfoOpenFullPath(fmt_c_str("**/###%", id.as_u64()));
            IM_CHECK_NE(info->ID, 0);
            IM_CHECK_STR_EQ(info->DebugLabel, fmt_c_str(ICON_FA_CUBE " shalg###%", id.as_u64()));
        }
    };
#endif
}

}
