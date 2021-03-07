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

#include "Editor.h"

#include "UiManager.h"

#include <yave/assets/SQLiteAssetStore.h>

namespace editor {

static Editor* editor_instance = nullptr;

Editor* Editor::instance() {
    y_debug_assert(editor_instance);
    return editor_instance;
}

Editor* Editor::init(DevicePtr dptr) {
    y_always_assert(!editor_instance, "Editor already initialized.");
    return editor_instance = new Editor(dptr);
}

void Editor::destroy() {
    y_always_assert(editor_instance, "No editor.");
    delete editor_instance;
    editor_instance = nullptr;
}


Editor::Editor(DevicePtr dptr) : DeviceLinked(dptr) {
    _ui = std::make_unique<UiManager>();
    _asset_store = std::make_unique<SQLiteAssetStore>("../store.sqlite3");
}

Editor::~Editor() {
}


}
