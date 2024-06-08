/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "SceneVisibilitySubPass.h"

namespace yave {

SceneVisibilitySubPass SceneVisibilitySubPass::create(const SceneView& scene_view) {
    const Scene* scene = scene_view.scene();

    SceneVisibilitySubPass pass;
    pass.scene_view = scene_view;
    pass.visible = std::make_shared<SceneVisibility>();

    pass.visible->meshes = scene->gather_visible(scene->meshes(), scene_view.camera(), scene_view.visibility_mask());
    pass.visible->point_lights = scene->gather_visible(scene->point_lights(), scene_view.camera(), scene_view.visibility_mask());
    pass.visible->spot_lights = scene->gather_visible(scene->spot_lights(), scene_view.camera(), scene_view.visibility_mask());

    return pass;
}

}

