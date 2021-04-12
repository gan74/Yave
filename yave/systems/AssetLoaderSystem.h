/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

namespace yave {

class AssetLoaderSystem : public ecs::System {
    public:
        AssetLoaderSystem(AssetLoader& loader);

        void setup(ecs::EntityWorld& world) override;
        void tick(ecs::EntityWorld& world) override;

    private:
        void run_tick(ecs::EntityWorld& world, bool only_recent);
        void post_load(ecs::EntityWorld& world);

        core::ExternalHashMap<ecs::ComponentTypeIndex, core::Vector<ecs::EntityId>> _loading;

        AssetLoader* _loader = nullptr;




    private:
        template<typename CRTP>
        friend class HasLoadableAssetsTag;

        struct LoadableComponentTypeInfo {
            void (*start_loading)(ecs::EntityWorld&, AssetLoadingContext&, bool, core::Vector<ecs::EntityId>&) = nullptr;
            void (*update_status)(ecs::EntityWorld&, core::Vector<ecs::EntityId>&) = nullptr;
            ecs::ComponentTypeIndex type;
            LoadableComponentTypeInfo* next = nullptr;
        };

        template<typename T>
        static void register_loadable_component_type() {
            static LoadableComponentTypeInfo info = LoadableComponentTypeInfo {
                &start_loading_components<T>,
                &update_loading_status<T>,
                ecs::type_index<T>(),
                _first_info
            };
            _first_info = &info;
        }

        template<typename T>
        static core::Span<ecs::EntityId> ids(ecs::EntityWorld& world, bool recent) {
            return recent
                ? world.recently_added<T>()
                : world.component_ids<T>();
        }

        template<typename T>
        static auto view(ecs::EntityWorld& world, bool recent) {
            return world.view<T>(ids<T>(world, recent));
        }

        template<typename T>
        static void start_loading_components(ecs::EntityWorld& world, AssetLoadingContext& loading_ctx, bool recent, core::Vector<ecs::EntityId>& out_ids) {
            for(auto id_comp : view<T>(world, recent).id_components()) {
                id_comp.component<T>().load_assets(loading_ctx);
                out_ids << id_comp.id();
            }
        }

        template<typename T>
        static void update_loading_status(ecs::EntityWorld& world, core::Vector<ecs::EntityId>& ids) {
            for(usize i = 0; i != ids.size(); ++i) {
                T* component = world.component<T>(ids[i]);
                if(component->update_asset_loading_status()) {
                    ids.erase_unordered(ids.begin() + i);
                    --i;
                }
            }
        }

        static LoadableComponentTypeInfo* _first_info;
};

}

#endif // YAVE_SYSTEMS_ASSETLOADERSYSTEM_H

