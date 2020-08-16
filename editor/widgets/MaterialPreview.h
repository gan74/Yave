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
#ifndef EDITOR_MATERIALPREVIEW_H
#define EDITOR_MATERIALPREVIEW_H

#include <editor/ui/Widget.h>

#include <yave/assets/AssetPtr.h>
#include <yave/scene/SceneView.h>

namespace editor {

class MaterialPreview final : public Widget, public ContextLinked {
    public:
        enum class PreviewObject {
            Sphere,
            Cube,
        };

        MaterialPreview(ContextPtr cptr);

        void refresh() override;

        void set_material(const AssetPtr<Material>& material);

        void set_object(const AssetPtr<StaticMesh>& mesh);
        void set_object(PreviewObject obj);

        void paint(CmdBufferRecorder& recorder) override;

    private:
        void paint_mesh_menu();

        void reset_world();

        void update_camera();

        AssetPtr<Material> _material;
        AssetPtr<StaticMesh> _mesh;

        std::unique_ptr<EditorWorld> _world;
        SceneView _view;

        AssetPtr<IBLProbe> _ibl_probe;
        std::shared_ptr<FrameGraphResourcePool> _resource_pool;

        float _cam_distance = 1.0f;
        math::Vec2 _angle = math::Vec2(math::to_rad(45.0f));

};

}


#endif // EDITOR_MATERIALPREVIEW_H

