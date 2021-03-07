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

#include "AssetSelector.h"

#include <editor/utils/assets.h>

namespace editor {

AssetSelector::AssetSelector(AssetType filter) :
        ResourceBrowser(fmt("% Asset selector", asset_type_icon(filter))),
        _filter(filter) {
}

void AssetSelector::asset_selected(AssetId id) {
    if(id != AssetId::invalid_id()) {
        if(read_file_type(id) == _filter) {
            if(_selected(id)) {
                close();
                return;
            }
        }
    }
}

core::Result<core::String> AssetSelector::entry_icon(const core::String& full_name, EntryType type) const {
    if(type == EntryType::Directory) {
        return FileSystemView::entry_icon(full_name, type);
    }
    if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
        const AssetType asset = read_file_type(id);
        if(_filter == AssetType::Unknown || asset == _filter) {
            return core::Ok(core::String(asset_type_icon(asset)));
        }
    }
    return core::Err();
}


}

