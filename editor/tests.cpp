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

#include <yave/ecs/EntityWorld.h>

#include <external/imgui_test_engine/imgui_te_context.h>

#include <y/utils/log.h>

namespace editor {

static core::Vector<core::String> window_names() {
    core::Vector<core::String> names;
    const auto& windows = ImGui::GetCurrentContext()->Windows;
    std::transform(windows.begin(), windows.end(), std::back_inserter(names), [](const ImGuiWindow* w) { return w->Name; });
    return names;
}

void register_editor_tests(ImGuiTestEngine* engine) {
    log_msg("Registering ImGui tests", Log::Debug);

    IM_REGISTER_TEST(engine, "tests", "new scene")->TestFunc = [](ImGuiTestContext* ctx) {
        ctx->SetRef("##MainMenuBar");
        ctx->MenuClick("File/New");
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
}

}
