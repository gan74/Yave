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

#include "AssetDependencies.h"

namespace yave {

AssetDependencies::AssetDependencies(AssetLoadingFlags flags) : _flags(flags) {
}

void AssetDependencies::add_dependency(GenericAssetPtr asset) {
    if(asset.id() != AssetId::invalid_id()) {
        _deps.emplace_back(std::move(asset));
    }
}

usize AssetDependencies::dependency_count() const {
    return _deps.size();
}

bool AssetDependencies::is_done() const {
    return state() != AssetLoadingState::NotLoaded;
}

bool AssetDependencies::is_empty() const {
    return _deps.is_empty();
}

AssetLoadingState AssetDependencies::state() const {
    for(const auto& d : _deps) {
        if(!d.is_loaded()) {
            if(!d.is_failed()) {
                return AssetLoadingState::NotLoaded;
            }
            const bool fails_deps = (_flags & AssetLoadingFlags::SkipFailedDependenciesBit) == AssetLoadingFlags::SkipFailedDependenciesBit;
            if(!fails_deps) {
                return AssetLoadingState::Failed;
            }
        }
    }
    return AssetLoadingState::Loaded;
}

AssetLoadingErrorType AssetDependencies::error() const {
    for(const auto& d : _deps) {
        if(!d.is_failed()) {
            return d.error();
        }
    }
    return AssetLoadingErrorType::Unknown;
}

}

