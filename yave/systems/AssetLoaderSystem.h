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
#ifndef YAVE_SYSTEMS_ASSETLOADERSYSTEM_H
#define YAVE_SYSTEMS_ASSETLOADERSYSTEM_H

#include <yave/ecs/EntityWorld.h>

#include <y/core/Vector.h>
#include <y/core/HashMap.h>


namespace yave {

class AssetLoaderSystem : public ecs::System {
    public:
        AssetLoaderSystem(AssetLoader& loader);

        void setup(ecs::SystemScheduler& sched) override;

        template<typename T>
        void register_component_type() {
            _infos << LoadableComponentTypeInfo {
                ct_type_name<T>(),
                &load_components<T, false>,
                &load_components<T, true>,
                &update_loading_status<T>
            };
        }

    private:
        struct LoadableComponentTypeInfo {
            std::string_view type_name;
            void (*load_all)(ecs::EntityWorld&, AssetLoadingContext&) = nullptr;
            void (*load_recent)(ecs::EntityWorld&, AssetLoadingContext&) = nullptr;
            void (*update_status)(ecs::EntityWorld&) = nullptr;
        };

        template<typename T>
        struct LoadingTag {};

        template<typename T, bool Recent>
        static void load_components(ecs::EntityWorld& world, AssetLoadingContext& loading_ctx) {
            auto query = [&] {
                if constexpr(Recent) {
                    Y_TODO(hoist group into system schedule)
                    return world.create_group<ecs::Mutate<ecs::Changed<T>>>().query();
                } else {
                    return world.create_group<ecs::Mutate<T>>().query();
                }
            }();

            if(query.is_empty()) {
                return;
            }

            y_profile_msg(fmt_c_str("Processing {} components", query.size()));
            for(auto&& [id, comp] : query.id_components()) {
                comp.load_assets(loading_ctx);
                world.get_or_add_component<LoadingTag<T>>(id);
            }
        }

        template<typename T>
        static void update_loading_status(ecs::EntityWorld& world) {
            auto query = world.create_group<ecs::Mutate<T>, LoadingTag<T>>().query();
            for(auto&& [id, comp, loading] : query.id_components()) {
                if(comp.update_asset_loading_status()) {
                    world.remove_component<LoadingTag<T>>(id);
                }
            }

        }

        core::Vector<LoadableComponentTypeInfo> _infos;
        AssetLoader* _loader = nullptr;

};

}

#endif // YAVE_SYSTEMS_ASSETLOADERSYSTEM_H

