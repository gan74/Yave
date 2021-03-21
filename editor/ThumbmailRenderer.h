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

#ifndef EDITOR_THUMBMAILRENDERER_H
#define EDITOR_THUMBMAILRENDERER_H

#include <editor/editor.h>

#include <yave/graphics/images/ImageView.h>
#include <yave/assets/AssetPtr.h>

#include <y/core/HashMap.h>
#include <y/concurrent/StaticThreadPool.h>

#include <mutex>

namespace editor {

class ThumbmailRenderer : NonMovable {

    struct ThumbmailData : NonMovable {
        Texture texture;
        TextureView view;
        GenericAssetPtr asset_ptr;
        std::function<Texture()> render = nullptr;
        // To avoid futures...
        concurrent::DependencyGroup done;
        std::atomic<bool> failed = false;
    };

    public:
        static constexpr usize thumbmail_size = 256;

        ThumbmailRenderer(AssetLoader& loader);

        const TextureView* thumbmail(AssetId id);

    private:
        void query(AssetId id, ThumbmailData& data);

        core::ExternalHashMap<AssetId, std::unique_ptr<ThumbmailData>> _thumbmails;
        AssetLoader* _loader = nullptr;

        std::mutex _lock;
        concurrent::WorkerThread _render_thread;
};

}

#endif // EDITOR_THUMBMAILRENDERER_H
