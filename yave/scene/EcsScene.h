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
#ifndef YAVE_SCENE_ECSSCENE_H
#define YAVE_SCENE_ECSSCENE_H

#include "Scene.h"

#include <yave/ecs/SparseComponentSet.h>

namespace yave {

class EcsScene : public Scene {
    struct ObjectIndices {
        u32 mesh = u32(-1);
        u32 point_light = u32(-1);
        u32 spot_light = u32(-1);
        u32 directional_light = u32(-1);
        u32 sky_light = u32(-1);
    };

    public:
        EcsScene(const ecs2::EntityWorld* w);

        const ecs2::EntityWorld* world() const;

        void update_from_world();

    private:
        template<typename S>
        typename S::value_type& register_object(u32& index, S& storage);

        template<typename T, typename S>
        void process_transformable_components(u32 ObjectIndices::* index, S& storage);

        template<typename T, typename S>
        void process_components(u32 ObjectIndices::* index, S& storage);

        const ecs2::EntityWorld* _world = nullptr;

        ecs::SparseComponentSet<ObjectIndices> _indices;
};

}

#endif // YAVE_SCENE_ECSSCENE_H

