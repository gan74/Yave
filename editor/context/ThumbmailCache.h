/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef EDITOR_UTILS_THUMBMAILCACHE_H
#define EDITOR_UTILS_THUMBMAILCACHE_H

#include <editor/editor.h>
#include <yave/renderer/LightingPass.h>

#include <future>

namespace editor {

class ThumbmailCache : NonCopyable, public ContextLinked {

		struct Thumbmail {
			Thumbmail(DevicePtr dptr, usize size, AssetId asset);

			Image<ImageUsage::StorageBit | ImageUsage::TextureBit> image;
			TextureView view;
			AssetId id;
		};

		struct SceneData : NonMovable {
			SceneData(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat);

			Scene scene;
			SceneView view;
		};

		using ThumbmailFunc = core::Functor<std::unique_ptr<Thumbmail>(CmdBufferRecorder&)>;

	public:
		using ThumbmailView = std::reference_wrapper<const TextureView>;

		ThumbmailCache(ContextPtr ctx, usize size = 256);

		math::Vec2ui thumbmail_size() const;

		TextureView* get_thumbmail(AssetId asset);

		void clear();

	private:
		void process_requests();
		void request_thumbmail(AssetId id);
		std::unique_ptr<Thumbmail> render_thumbmail(CmdBufferRecorder& recorder, const AssetPtr<Texture>& tex) const;
		std::unique_ptr<Thumbmail> render_thumbmail(CmdBufferRecorder& recorder, AssetId id, const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat) const;

		usize _size;

		std::shared_ptr<IBLData> _ibl_data;
		std::unordered_map<AssetId, std::unique_ptr<Thumbmail>> _thumbmails;
		core::Vector<std::future<ThumbmailFunc>> _requests;
};

}

#endif // EDITOR_UTILS_THUMBMAILCACHE_H
