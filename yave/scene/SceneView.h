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
#ifndef YAVE_SCENE_SCENEVIEW_H
#define YAVE_SCENE_SCENEVIEW_H

#include "Scene.h"

#include <yave/camera/Camera.h>

namespace yave {

class SceneView {
    public:
        SceneView() = default;
        SceneView(const Scene* sce, Camera cam = Camera(), u32 visibility = u32(-1));

        const Scene* scene() const;

        const Camera& camera() const;
        Camera& camera();

        u32 visibility_mask() const;

    private:
        const Scene* _scene = nullptr;
        Camera _camera;
        u32 _visility_mask = u32(-1);
};

}

#endif // YAVE_SCENE_SCENEVIEW_H

