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

#include "editor.h"

#include "EditorApplication.h"
#include "UiManager.h"

#include "Settings.h"
#include "Selection.h"


namespace editor {

EditorApplication* application() {
    return EditorApplication::instance();
}

DevicePtr app_device() {
    return application()->device();
}


Settings& app_settings() {
    static Settings settings;
    return settings;
}

Selection& selection() {
    static Selection select;
    return select;
}


AssetStore& asset_store() {
    return application()->asset_store();
}

AssetLoader& asset_loader() {
    return application()->asset_loader();
}

EditorWorld& world() {
    return application()->world();
}

const SceneView& scene_view() {
    return application()->scene_view();
}

const EditorResources& resources() {
    return application()->resources();
}

UiManager& ui() {
    return application()->ui();
}

ImGuiPlatform* imgui_platform() {
    return application()->imgui_platform();
}


Widget* add_widget(std::unique_ptr<Widget> widget, bool auto_parent) {
    return ui().add_widget(std::move(widget), auto_parent);
}

}


