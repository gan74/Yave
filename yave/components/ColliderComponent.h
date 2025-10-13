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
#ifndef YAVE_COMPONENTS_COLLIDERCOMPONENT_H
#define YAVE_COMPONENTS_COLLIDERCOMPONENT_H

#include <yave/ecs/ecs.h>

#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>


namespace yave {

class ColliderComponent final {

    public:
        enum class Type {
            Static,
            Kinematic,
            Dynamic,
        };

        ColliderComponent() = default;

        void set_type(Type t) {
            _type = t;
        }

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(ColliderComponent, _type)

    private:
        friend class JoltPhysicsSystem;

        static constexpr u32 invalid_index = u32(-1);
        
        Type _type = Type::Static;

        mutable u32 _body_id = invalid_index;
        mutable math::Vec3 _scale = math::Vec3(1.0f);
};


}

#endif // YAVE_COMPONENTS_COLLIDERCOMPONENT_H

