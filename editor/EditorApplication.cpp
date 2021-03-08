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

#include "EditorApplication.h"

#include "UiManager.h"

#include <yave/assets/SQLiteAssetStore.h>

namespace editor {

EditorApplication* EditorApplication::_instance = nullptr;

EditorApplication* EditorApplication::instance() {
    y_debug_assert(_instance);
    return _instance;
}

EditorApplication::EditorApplication(DevicePtr dptr) : DeviceLinked(dptr) {
    y_always_assert(_instance == nullptr, "Editor instance already exists.");
    _instance = this;

    _ui = std::make_unique<UiManager>();
    _asset_store = std::make_unique<SQLiteAssetStore>("../store.sqlite3");
}

EditorApplication::~EditorApplication() {
    y_always_assert(_instance == this, "Editor instance isn't set propertly.");
}


}
