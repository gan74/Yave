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
#ifndef YAVE_ECS_COMPONENTINSPECTOR_H
#define YAVE_ECS_COMPONENTINSPECTOR_H

#include "ecs.h"

#include <yave/assets/AssetPtr.h>

#include <y/core/String.h>
#include <y/utils/format.h>

#include <numeric>

namespace yave {
namespace ecs {

class ComponentInspector : NonCopyable {
    public:
        enum class Vec3Role {
            None,
            Position,
            Direction,
            Color,
        };

        enum class FloatRole {
            None,
            Angle,                  // radian
            HalfAngle,              // radian/2
            Distance,               // m
            DistanceKilometers,     // km
            NormalizedLumFlux,      // lm/4pi
            Illuminance,            // lm/m2
        };

        virtual ~ComponentInspector();

        virtual bool inspect_component_type(ComponentRuntimeInfo info, bool has_inspect) = 0;

        virtual void inspect(const core::String& name, math::Transform<>& t) = 0;

        virtual void inspect(const core::String& name, math::Vec3& v, Vec3Role role = Vec3Role::None) = 0;

        virtual void inspect(const core::String& name, float& f, FloatRole role = FloatRole::None);
        virtual void inspect(const core::String& name, float& f, float min, float max = std::numeric_limits<float>::max(), FloatRole role = FloatRole::None) = 0;

        virtual void inspect(const core::String& name, u32& u, u32 max = u32(-1)) = 0;

        virtual void inspect(const core::String& name, bool& b) = 0;

        virtual void inspect(const core::String& name, GenericAssetPtr& p) = 0;


        template<typename T, typename... Args>
        void inspect(const core::String& name, core::MutableSpan<T> items, Args&&... args) {
            if(begin_collection(name)) {
                for(usize i = 0; i != items.size(); ++i) {
                    inspect(fmt("%[%]", name, i), items[i], y_fwd(args)...);
                }
                end_collection();
            }
        }

        template<typename T>
        void inspect(const core::String& name, AssetPtr<T>& ptr) {
            GenericAssetPtr gen(ptr);
            inspect(name, gen);
            ptr = gen.to<T>();
            y_debug_assert(gen == ptr);
        }


    protected:
        virtual bool begin_collection(const core::String&) {
            return true;
        }

        virtual void end_collection() {
        }

};

}
}

#endif // YAVE_ECS_COMPONENTINSPECTOR_H

