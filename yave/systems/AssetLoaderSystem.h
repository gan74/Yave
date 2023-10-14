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
#ifndef YAVE_SYSTEMS_ASSETLOADERSYSTEM_H
#define YAVE_SYSTEMS_ASSETLOADERSYSTEM_H

#include <yave/ecs/EntityWorld.h>

#include <y/core/Vector.h>
#include <y/core/HashMap.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

class AssetLoaderSystem : public ecs::System {
    public:
        AssetLoaderSystem(AssetLoader& loader);

        void setup() override;
        void tick() override;


        template<typename T>
        void register_component_type() {
            _per_type_infos << LoadableComponentTypeInfo {
                ecs::type_index<T>(),
                &start_loading<T>,
                &update_status<T>,
                &connect<T>,
            };
        }


    private:
        struct LoadableComponentTypeInfo {
            ecs::ComponentTypeIndex type;

            void (*start_loading)(ecs::EntityWorld&, AssetLoadingContext&, core::Span<ecs::EntityId>) = nullptr;
            void (*update_status)(ecs::EntityWorld&, core::Vector<ecs::EntityId>&) = nullptr;
            void (*connect)(LoadableComponentTypeInfo&, ecs::EntityWorld&) = nullptr;

            core::Vector<ecs::EntityId> to_load;
            core::Vector<ecs::EntityId> loading;

            concurrent::Subscription created;
        };

        template<typename T>
        static void start_loading(ecs::EntityWorld& world, AssetLoadingContext& loading_ctx, core::Span<ecs::EntityId> to_load) {
            y_profile();

            for(const ecs::EntityId id : to_load) {
                world.patch<T>(id, [&](T& comp) {
                    comp.load_assets(loading_ctx);
                });
            }
        }

        template<typename T>
        static void update_status(ecs::EntityWorld& world, core::Vector<ecs::EntityId>& loading) {
            y_profile();

            for(usize i = 0; i != loading.size(); ++i) {
                world.patch<T>(loading[i], [&](T& comp) {
                    if(comp.update_asset_loading_status()) {
                        loading.erase_unordered(loading.begin() + i);
                        --i;
                    }
                });
            }
        }

        template<typename T>
        static void connect(LoadableComponentTypeInfo& info, ecs::EntityWorld& world) {
            info.created = world.on_component_created<T>().subscribe([&info](ecs::EntityId id, T&) {
                info.to_load.push_back(id);
            });
        }

        core::Vector<LoadableComponentTypeInfo> _per_type_infos;
        AssetLoader* _loader = nullptr;
};

}

#endif // YAVE_SYSTEMS_ASSETLOADERSYSTEM_H

