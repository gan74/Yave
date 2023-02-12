/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include <yave/utils/FileSystemModel.h>

#include <y/io2/File.h>
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
    for(const ecs::EntityId id : world.ids()) {
        entities << id;
    }
    return entities;
}


static constexpr std::string_view cube_gltf = R"#({"asset":{"generator":"Khronos glTF Blender I/O v3.4.50","version":"2.0"},"scene":0,"scenes":[{"name":"Scene","nodes":[0]}],"nodes":[{"mesh":0,"name":"Cube"}],"materials":[{"doubleSided":true,"name":"Material","pbrMetallicRoughness":{"baseColorFactor":[0.800000011920929,0.800000011920929,0.800000011920929,1],"metallicFactor":0,"roughnessFactor":0.5}}],"meshes":[{"name":"Cube","primitives":[{"attributes":{"POSITION":0,"TEXCOORD_0":1,"NORMAL":2},"indices":3,"material":0}]}],"accessors":[{"bufferView":0,"componentType":5126,"count":24,"max":[1,1,1],"min":[-1,-1,-1],"type":"VEC3"},{"bufferView":1,"componentType":5126,"count":24,"type":"VEC2"},{"bufferView":2,"componentType":5126,"count":24,"type":"VEC3"},{"bufferView":3,"componentType":5123,"count":36,"type":"SCALAR"}],"bufferViews":[{"buffer":0,"byteLength":288,"byteOffset":0,"target":34962},{"buffer":0,"byteLength":192,"byteOffset":288,"target":34962},{"buffer":0,"byteLength":288,"byteOffset":480,"target":34962},{"buffer":0,"byteLength":72,"byteOffset":768,"target":34963}],"buffers":[{"byteLength":840,"uri":"data:application/octet-stream;base64,AACAPwAAgD8AAIC/AACAPwAAgD8AAIC/AACAPwAAgD8AAIC/AACAPwAAgL8AAIC/AACAPwAAgL8AAIC/AACAPwAAgL8AAIC/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgL8AAIA/AACAPwAAgL8AAIA/AACAPwAAgL8AAIA/AACAvwAAgD8AAIC/AACAvwAAgD8AAIC/AACAvwAAgD8AAIC/AACAvwAAgL8AAIC/AACAvwAAgL8AAIC/AACAvwAAgL8AAIC/AACAvwAAgD8AAIA/AACAvwAAgD8AAIA/AACAvwAAgD8AAIA/AACAvwAAgL8AAIA/AACAvwAAgL8AAIA/AACAvwAAgL8AAIA/AAAgPwAAAD8AACA/AAAAPwAAID8AAAA/AADAPgAAAD8AAMA+AAAAPwAAwD4AAAA/AAAgPwAAgD4AACA/AACAPgAAID8AAIA+AADAPgAAgD4AAMA+AACAPgAAwD4AAIA+AAAgPwAAQD8AACA/AABAPwAAYD8AAAA/AAAAPgAAAD8AAMA+AABAPwAAwD4AAEA/AAAgPwAAAAAAACA/AACAPwAAYD8AAIA+AAAAPgAAgD4AAMA+AAAAAAAAwD4AAIA/AAAAAAAAAAAAAIC/AAAAAAAAgD8AAACAAACAPwAAAAAAAACAAAAAAAAAgL8AAACAAAAAAAAAAAAAAIC/AACAPwAAAAAAAACAAAAAAAAAAAAAAIA/AAAAAAAAgD8AAACAAACAPwAAAAAAAACAAAAAAAAAgL8AAACAAAAAAAAAAAAAAIA/AACAPwAAAAAAAACAAACAvwAAAAAAAACAAAAAAAAAAAAAAIC/AAAAAAAAgD8AAACAAAAAAAAAgL8AAACAAACAvwAAAAAAAACAAAAAAAAAAAAAAIC/AAAAAAAAAAAAAIA/AACAvwAAAAAAAACAAAAAAAAAgD8AAACAAAAAAAAAgL8AAACAAAAAAAAAAAAAAIA/AACAvwAAAAAAAACAAQAOABQAAQAUAAcACgAGABIACgASABYAFwATAAwAFwAMABAADwADAAkADwAJABUABQACAAgABQAIAAsAEQANAAAAEQAAAAQA"}]})#";

// https://github.com/ocornut/imgui_test_engine/wiki/Named-References
void register_editor_tests(ImGuiTestEngine* engine) {
    log_msg("Registering ImGui tests", Log::Debug);

#if 1
    IM_REGISTER_TEST(engine, "tests", "new scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("File/New");
        IM_CHECK_EQ(entity_count(), 0_uu);
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
        IM_CHECK_EQ(items.size(), 1_uu);

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
        IM_CHECK_NE(ids.size(), 0_uu);
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
        IM_CHECK_NE(ids.size(), 0_uu);
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
        IM_CHECK_EQ(entity_count(), 0_uu);

        ctx->MenuClick("File/" ICON_FA_FOLDER " Load");
        IM_CHECK_EQ(entity_count(), 3_uu);
    };

#if 0
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
#endif

    IM_REGISTER_TEST(engine, "tests", "add static mesh")->TestFunc = [](ImGuiTestContext* ctx) {
        const auto ids = all_ids();
        IM_CHECK_NE(ids.size(), 0_uu);
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
                IM_CHECK_NE(ctx->ItemInfoOpenFullPath("**/" ICON_FA_FOLDER_OPEN)->ID, 0u);
                ctx->ItemClose("**/" ICON_FA_PUZZLE_PIECE " StaticMeshComponent");
            }
        }

        ctx->Yield();

        {
            ctx->SetRef("//" ICON_FA_CUBES " Entities##1");
            const ImGuiTestItemInfo* info = ctx->ItemInfoOpenFullPath(fmt_c_str("**/###%", id.as_u64()));
            IM_CHECK_NE(info->ID, 0u);
            IM_CHECK_STR_EQ(info->DebugLabel, fmt_c_str(ICON_FA_CUBE " shalg###%", id.as_u64()));
        }
    };

    IM_REGISTER_TEST(engine, "tests", "import gltf")->TestFunc = [](ImGuiTestContext* ctx) {
        const core::String temp_file = core::String(std::tmpnam(nullptr)) + ".gltf";
        const auto path = FileSystemModel::local_filesystem()->parent_path(temp_file);
        IM_CHECK_EQ(path.is_error(), false);

        {
            auto file = io2::File::create(temp_file);
            IM_CHECK_EQ(file.is_error(), false);
            IM_CHECK_EQ(file.unwrap().write(cube_gltf.data(), cube_gltf.size()).is_error(), false);
        }

        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("View/ResourceBrowser");

        ctx->Yield();

        ctx->SetRef("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");
        ctx->ItemClick("**/" ICON_FA_PLUS " Import");
        ctx->ItemClick("//$FOCUSED/Import objects");

        IM_CHECK_EQ(FileSystemModel::local_filesystem()->exists(temp_file).unwrap_or(false), true);

        ctx->SetRef("//Scene importer##1");
        ctx->ItemInputValue("**/##path", path.unwrap().data());
        ctx->ItemInputValue("**/##filename", temp_file.sub_str(path.unwrap().size() + 1).data());

        for(usize i = 0; !ctx->ItemInfoOpenFullPath("**/" ICON_FA_CHECK " Import", ImGuiTestOpFlags_NoError)->ID; ++i) {
            ctx->Yield();
            IM_CHECK_NE(i, 1000_uu);
        }

        {
            ctx->ItemClick("**/" ICON_FA_FOLDER_OPEN);

            ctx->SetRef("//File browser##1");
            const ImGuiTestItemInfo* info = ctx->ItemInfoOpenFullPath("");
            IM_CHECK_NE(info->ID, 0u);
            log_msg(info->DebugLabel);
            ctx->MouseMoveToPos(info->RectClipped.GetCenter());
            ctx->MouseClick(ImGuiMouseButton_Right);
            ctx->ItemClick("//$FOCUSED/New folder");
            ctx->ItemInputValue("**/##path", "new folder");
            ctx->ItemClick("**/Ok");
        }


        ctx->SetRef("//Scene importer##1");
        ctx->ItemClick("**/" ICON_FA_CHECK " Import");

        for(usize i = 0; !ctx->ItemInfoOpenFullPath("**/Ok", ImGuiTestOpFlags_NoError)->ID; ++i) {
            ctx->Yield();
            IM_CHECK_NE(i, 1000_uu);
        }

        ctx->ItemClick("**/Ok");

        ctx->Yield();

        ctx->WindowClose("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");
    };


    IM_REGISTER_TEST(engine, "tests", "delete resource folder")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("View/ResourceBrowser");

        ctx->Yield();

        ctx->SetRef("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");
        ctx->ItemClick("**/" ICON_FA_FOLDER " new folder##0", ImGuiMouseButton_Right);
        ctx->ItemClick("//$FOCUSED/Delete");

        ctx->WindowClose("//" ICON_FA_FOLDER_OPEN " Resource Browser##1");
    };
#endif
}

}
