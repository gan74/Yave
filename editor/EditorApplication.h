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

#ifndef EDITOR_EDITORAPPLICATION_H
#define EDITOR_EDITORAPPLICATION_H

#include <editor/editor.h>

#include <yave/scene/SceneView.h>
#include <yave/graphics/graphics.h>

namespace editor {

class EditorApplication : NonMovable {
    public:
        EditorApplication(ImGuiPlatform* platform);
        ~EditorApplication();

        static EditorApplication* instance();

        void exec();

        void flush_reload();

        void set_scene_view(SceneView* scene);
        void unset_scene_view(SceneView* scene);


        void save_world();
        void load_world();


        SceneView& scene_view() {
            return *_scene_view;
        }

        AssetStore& asset_store() {
            return *_asset_store;
        }

        AssetLoader& asset_loader() {
            return *_loader;
        }

        ThumbmailRenderer& thumbmail_renderer() {
            return *_thumbmail_renderer;
        }

        EditorWorld& world() {
            return *_world;
        }

        const EditorResources& resources() const {
            return *_resources;
        }

        UiManager& ui() {
            return *_ui;
        }

        CmdBufferRecorder& recorder() {
            y_debug_assert(_recorder);
            return *_recorder;
        }

    private:
        static EditorApplication* _instance;

        void process_deferred_actions();
        void save_world_deferred() const;
        void load_world_deferred();


        ImGuiPlatform* _platform = nullptr;

        std::unique_ptr<EditorResources> _resources;

        std::shared_ptr<AssetStore> _asset_store;
        std::unique_ptr<AssetLoader> _loader;
        std::unique_ptr<ThumbmailRenderer> _thumbmail_renderer;

        std::unique_ptr<EditorWorld> _world;


        std::unique_ptr<UiManager> _ui;

        CmdBufferRecorder* _recorder = nullptr;

        SceneView _default_scene_view;
        SceneView* _scene_view = nullptr;


        enum DeferredActions : u32 {
            None = 0x00,
            Save = 0x01,
            Load = 0x02,
        };

        u32 _deferred_actions = None;
};

}

#endif // EDITOR_EDITORAPPLICATION_H

