/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef EDITOR_WORLDWORKSPACE_H
#define EDITOR_WORLDWORKSPACE_H

#include "Workspace.h"
#include "EditorWorld.h"

#include <yave/scene/SceneView.h>
#include <yave/utils/DirectDraw.h>

#include <y/concurrent/JobSystem.h>

#include <memory>

namespace editor {

class WorldWorkspace final : public Workspace {
    public:
        WorldWorkspace();
        ~WorldWorkspace() override;

        void update() override;
        void post_update() override;

        std::string_view name() const override;

        void save() override;
        void load() override;

        void grab_reloaded() override;

        EditorWorld& world();
        const EditorWorld& world() const;

        const Scene& scene() const;

        void set_scene_view(SceneView* scene);
        void unset_scene_view(SceneView* scene);
        const SceneView& scene_view() const;

        DirectDraw& debug_drawer();

    private:
        void create_default_scene_view();
        void save_world_deferred();
        void load_world_deferred();

        enum DeferredActions : u32 {
            None    = 0x00,
            Save    = 0x01,
            Load    = 0x02,
            New     = 0x04,
        };

        std::unique_ptr<EditorWorld> _world;
        std::unique_ptr<concurrent::JobSystem> _job_system;
        std::unique_ptr<DirectDraw> _debug_drawer;

        SceneView _default_scene_view;
        SceneView* _scene_view = nullptr;

        u32 _deferred_actions = None;
};

}

#endif // EDITOR_WORLDWORKSPACE_H
