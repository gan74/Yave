/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include <y/core/Functor.h>

#include <yave/assets/AssetPtr.h>
#include <yave/graphics/images/ImageView.h>

#include <yave/ecs/EntityWorld.h>
#include <yave/scene/SceneView.h>

#include <yave/framegraph/FrameGraphResourcePool.h>

#include <future>

namespace editor {

class ThumbmailCache : NonCopyable, public ContextLinked {

		struct ThumbmailData {
			ThumbmailData(DevicePtr dptr, usize size, AssetId asset);

			StorageTexture image;
			TextureView view;
			const AssetId id;

			core::Vector<std::pair<core::String, core::String>> properties;
		};

		struct SceneData : NonMovable {
			SceneData(ContextPtr ctx, const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat);

			ecs::EntityWorld world;
			SceneView view;
		};

		using ThumbmailFunc = core::Functor<std::unique_ptr<ThumbmailData>(CmdBufferRecorder&)>;

	public:
		using ThumbmailView = std::reference_wrapper<const TextureView>;

		struct Thumbmail {
			TextureView* image = nullptr;
			core::Span<std::pair<core::String, core::String>> properties;
		};

		ThumbmailCache(ContextPtr ctx, usize size = 256);

		math::Vec2ui thumbmail_size() const;

		Thumbmail get_thumbmail(AssetId asset);

		void clear();

	private:
		void process_requests();
		void request_thumbmail(AssetId id);
		std::unique_ptr<ThumbmailData> render_thumbmail(CmdBufferRecorder& recorder, const AssetPtr<Texture>& tex) const;
		std::unique_ptr<ThumbmailData> render_thumbmail(CmdBufferRecorder& recorder, AssetId id, const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat) const;

		usize _size;

		std::unordered_map<AssetId, std::unique_ptr<ThumbmailData>> _thumbmails;
		core::Vector<std::future<ThumbmailFunc>> _requests;

		std::shared_ptr<FrameGraphResourcePool> _resource_pool;
};

}

#endif // EDITOR_UTILS_THUMBMAILCACHE_H
