/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <y/core/HashMap.h>
#include <y/concurrent/StaticThreadPool.h>

#include <yave/assets/AssetPtr.h>
#include <yave/graphics/images/ImageView.h>

#include <yave/ecs/EntityWorld.h>
#include <yave/scene/SceneView.h>

#include <yave/framegraph/FrameGraphResourcePool.h>

#include <future>

namespace editor {

class ThumbmailCache : NonMovable, public ContextLinked {

		struct ThumbmailData {
			ThumbmailData(ContextPtr ctx, usize size, AssetId asset);

			StorageTexture image;
			TextureView view;
			const AssetId id;

			core::Vector<std::pair<core::String, core::String>> properties;
		};

		struct SceneData : NonMovable, public ContextLinked {
			SceneData(ContextPtr ctx);

			void add_mesh(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& mat);
			void add_prefab(const ecs::EntityPrefab& prefab);

			ecs::EntityWorld world;
			SceneView view;
		};

		using RenderFunc = std::function<std::unique_ptr<ThumbmailData>(CmdBufferRecorder&)>;

		struct LoadingRequest {
			GenericAssetPtr asset;
			RenderFunc func;
		};

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
		void request_thumbmail(AssetId id);

		void submit_and_set(CmdBufferRecorder& recorder, std::unique_ptr<ThumbmailData> thumb);

		std::unique_ptr<ThumbmailData> render_thumbmail(CmdBufferRecorder& recorder, const AssetPtr<Texture>& tex) const;
		std::unique_ptr<ThumbmailData> render_thumbmail(CmdBufferRecorder& recorder, AssetId id, const SceneData& scene) const;

		usize _size;

		std::shared_ptr<FrameGraphResourcePool> _resource_pool;
		core::ExternalHashMap<AssetId, std::unique_ptr<ThumbmailData>> _thumbmails;

		std::mutex _lock;
		concurrent::WorkerThread _render_thread = concurrent::WorkerThread("Thumbmail rendering thread");
};

}

#endif // EDITOR_UTILS_THUMBMAILCACHE_H
