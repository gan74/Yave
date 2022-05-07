/*******************************
Copyright (c) 2016-2022 Gr�goire Angerand

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
#ifndef YAVE_ECS_ENTITYSCENE_H
#define YAVE_ECS_ENTITYSCENE_H

#include "EntityPrefab.h"

#include <yave/assets/AssetPtr.h>

namespace yave {
namespace ecs {

class EntityScene {
    public:
        EntityScene() = default;
        EntityScene(core::Vector<AssetPtr<EntityPrefab>> prefabs) : _prefabs(std::move(prefabs)) {
        }


        y_reflect(_prefabs);
    private:
        core::Vector<AssetPtr<EntityPrefab>> _prefabs;
};

}

YAVE_DECLARE_GENERIC_ASSET_TRAITS(ecs::EntityScene, AssetType::Scene);

}

#endif // YAVE_ECS_SCENE_H

