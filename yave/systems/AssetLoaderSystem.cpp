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

#include "AssetLoaderSystem.h"

#include <yave/assets/AssetLoader.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

AssetLoaderSystem::AssetLoaderSystem(AssetLoader& loader) : ecs::System("AssetLoaderSystem"), _loader(&loader) {
}

void AssetLoaderSystem::setup() {
    for(LoadableComponentTypeInfo& info : _per_type_infos) {
        info.connect(info, world());
    }
}

void AssetLoaderSystem::tick() {
    y_profile();

    AssetLoadingContext loading_ctx(_loader);

    for(LoadableComponentTypeInfo& info : _per_type_infos) {
        y_debug_assert(info.created.is_connected());

        info.start_loading(world(), loading_ctx, info.to_load);
        info.loading.push_back(info.to_load.begin(), info.to_load.end());
        info.to_load.make_empty();
    }

    for(LoadableComponentTypeInfo& info : _per_type_infos) {
        info.update_status(world(), info.loading);
    }
}


}

