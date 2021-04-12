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

#include "AssetLoaderSystem.h"

#include <yave/assets/AssetLoader.h>

namespace yave {

AssetLoaderSystem::LoadableComponentTypeInfo* AssetLoaderSystem::_first_info = nullptr;

AssetLoaderSystem::AssetLoaderSystem(AssetLoader& loader) : ecs::System("AssetLoaderSystem"), _loader(&loader) {
}

void AssetLoaderSystem::setup(ecs::EntityWorld& world) {
    run_tick(world, false);
}

void AssetLoaderSystem::tick(ecs::EntityWorld& world) {
    run_tick(world, true);
}

void AssetLoaderSystem::run_tick(ecs::EntityWorld& world, bool only_recent) {
    y_profile();

    AssetLoadingContext loading_ctx(_loader);

    {
        for(const LoadableComponentTypeInfo* info = _first_info; info; info = info->next) {
            info->start_loading(world, loading_ctx, only_recent, _loading[info->type]);
        }
    }

    post_load(world);
}

void AssetLoaderSystem::post_load(ecs::EntityWorld& world) {
    y_profile();

    {
        for(const LoadableComponentTypeInfo* info = _first_info; info; info = info->next) {
            info->update_status(world, _loading[info->type]);
        }
    }
}


}

