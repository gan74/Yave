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
#ifndef YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H
#define YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H

#include <yave/ecs/ecs.h>
#include <y/reflect/reflect.h>

namespace yave {

class AtmosphereComponent final {
    public:
        float sea_level() const;

        ecs::EntityId sun() const;

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(AtmosphereComponent, _sea_level, _sun)

    private:
        float _sea_level = 0.0f;

        ecs::EntityId _sun;
};

}

#endif // YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H
