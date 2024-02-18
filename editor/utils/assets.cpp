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

#include "assets.h"

#include <y/utils/format.h>

#include <editor/utils/ui.h>

#include <cctype>

namespace editor {

std::string_view asset_type_name(AssetType type, bool plural, bool lowercase) {
    std::string_view name = asset_type_name(type);
    if(plural) {
        return fmt("{}{}{}", lowercase ? name[0] : std::toupper(name[0]), name.substr(1), name[name.size() - 1] == 'h' ? "es" : "s");
    }
    return name;
}


std::string_view asset_type_icon(AssetType type) {
    switch(type) {
        case AssetType::Image:
            return ICON_FA_IMAGE;

        case AssetType::Mesh:
            return ICON_FA_CUBE;

        case AssetType::Material:
            return ICON_FA_BRUSH;

        case AssetType::Prefab:
            return ICON_FA_DATABASE;

        default:
            return ICON_FA_QUESTION;
    }
    return ICON_FA_QUESTION;
}

}

