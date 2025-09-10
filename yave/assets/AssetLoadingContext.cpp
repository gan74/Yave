/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "AssetLoadingContext.h"

#include "AssetLoader.h"

namespace yave {

AssetLoadingContext::AssetLoadingContext(AssetLoader* loader, AssetLoadingFlags flags) : _parent(loader), _dependencies(flags | loader->loading_flags()) {
    y_always_assert(loader, "Invalid parent");
}

const AssetDependencies& AssetLoadingContext::dependencies() const {
    return _dependencies;
}

AssetLoader* AssetLoadingContext::parent() const {
    return _parent;
}

AssetLoadingFlags AssetLoadingContext::flags() const {
    return _dependencies.flags();
}

}

