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

#include "Scene.h"

namespace yave {

void Scene::update_tlas() {
    if(!raytracing_enabled()) {
        return;
    }

    y_profile();

    auto instances = core::Vector<BLASInstance>::with_capacity(_meshes.size() * 2);

    {
        y_profile_zone("gather instances");
        for(const StaticMeshObject& mesh_obj : _meshes) {
            if(const StaticMesh* mesh = mesh_obj.component.mesh().get()) {
                const math::Transform<>& tr = transform(mesh_obj);

                const auto blases = mesh->blases();
                const auto materials = mesh_obj.component.materials();
                if(materials.is_empty()) {
                    continue;
                }
                if(materials.size() == 1) {
                    if(const Material* material = materials[0].get()) {
                        const u32 index = material->draw_data().index();
                        for(usize i = 0; i != blases.size(); ++i) {
                            instances << TLAS::make_instance(tr, blases[i], index);
                        }
                    }
                } else {
                    y_debug_assert(blases.size() == materials.size());
                    for(usize i = 0; i != blases.size(); ++i) {
                        if(const Material* material = materials[i].get()) {
                            instances << TLAS::make_instance(tr, blases[i], material->draw_data().index());
                        }
                    }
                }
                
            }
        }
    }

    _tlas = TLAS(instances);
}

}

