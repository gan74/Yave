/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef EDITOR_CONTEXT_LOADER_H
#define EDITOR_CONTEXT_LOADER_H

#include <editor/editor.h>
#include <yave/assets/AssetLoader.h>
#include <yave/assets/FolderAssetStore.h>

#include <yave/images/Image.h>
#include <yave/meshes/StaticMesh.h>


namespace editor {

class Loader : public DeviceLinked {
	public:
		Loader(DevicePtr dptr) :
			DeviceLinked(dptr),
			_store(std::make_shared<FolderAssetStore>()),
			_textures(dptr, _store),
			_meshes(dptr, _store) {
		}

		AssetStore& asset_store() {
			return *_store;
		}

		AssetLoader<Texture>& texture() {
			return _textures;
		}

		AssetLoader<StaticMesh>& static_mesh() {
			return _meshes;
		}


	private:
		std::shared_ptr<AssetStore> _store;

		AssetLoader<Texture> _textures;
		AssetLoader<StaticMesh> _meshes;
};

}

#endif // EDITOR_CONTEXT_LOADER_H
