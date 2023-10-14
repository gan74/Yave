/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef YAVE_SYSTEMS_TRANSFORMMANAGERSYSTEM_H
#define YAVE_SYSTEMS_TRANSFORMMANAGERSYSTEM_H

#include <yave/ecs/System.h>

#include <y/concurrent/Signal.h>
#include <y/core/Vector.h>

namespace yave {

class TransformManagerSystem : public ecs::System {
    struct TransformData {
        math::Transform<> world_transform;

        bool modified = false;
    };

    public:
        TransformManagerSystem();

        void setup() override;

    private:
        friend class TransformableComponent;


        void update_world_transform(u32 index, const TransformableComponent* tr);

        void set_world_transform(TransformableComponent& tr, const math::Transform<>& world_transform);
        const math::Transform<>& world_transform(const TransformableComponent& tr) const;

    private:
        void make_modified(u32 index);


        concurrent::Subscription _entity_created;
        concurrent::Subscription _transform_created;

        core::Vector<TransformData> _transforms;
        core::Vector<u32> _modified;
};

}

#endif // YAVE_SYSTEMS_TRANSFORMMANAGERSYSTEM_H

