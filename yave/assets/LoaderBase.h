/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef YAVE_ASSETS_LOADERBASE_H
#define YAVE_ASSETS_LOADERBASE_H

#include "AssetId.h"

namespace yave {

template<typename T>
class AssetPtr;
template<typename T>
class AsyncAssetPtr;
template<typename T>
class Loader;
template<typename T>
class WeakAssetPtr;


enum class AssetLoadingErrorType : u32 {
	InvalidID,
	UnknownID,
	InvalidData,
	UnknownType,
	Unknown
};

class LoaderBase : NonMovable {
	public:
		using ErrorType = AssetLoadingErrorType;

		virtual ~LoaderBase();

		AssetLoader* parent() const;
		DevicePtr device() const;

		AssetStore& store();
		const AssetStore& store() const;

		virtual AssetType type() const = 0;

	protected:
		friend class AssetLoader;

		LoaderBase(AssetLoader* parent);

	private:

		AssetLoader* _parent = nullptr;
};

}

#endif // YAVE_ASSETS_LOADERBASE_H
