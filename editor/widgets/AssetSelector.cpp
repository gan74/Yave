/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "AssetSelector.h"

#include <editor/utils/assets.h>

#include <y/utils/format.h>

namespace editor {

AssetSelector::AssetSelector(AssetType filter, const char* name) :
        Widget(name ? std::string_view(name) : fmt("Select {}", asset_type_name(filter, false, true))),
        _filter(filter) {

    set_modal(true);

    _browser.set_filter_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) {
        if(type == FileSystemModel::EntryType::Directory) {
            return true;
        }
        if(const AssetId id = _browser.asset_id(full_name); id != AssetId::invalid_id()) {
            return _filter == AssetType::Unknown || _browser.asset_type(id) == _filter;
        }
        return false;
    });

    _browser.set_selected_delegate([this](AssetId id) {
        if(id != AssetId::invalid_id()) {
            if(_browser.asset_type(id) == _filter) {
                if(_selected(id)) {
                    close();
                    return true;
                }
            }
        }
        return false;
    });
}


void AssetSelector::on_gui() {
    _browser.draw_gui_inside();
}

}

