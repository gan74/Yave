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

#include <yave/ecs2/EntityWorld.h>

#include <y/core/Vector.h>
#include <y/core/HashMap.h>

#include <y/utils/format.h>

namespace yave {

class AssetLoaderSystem : public ecs2::System {
    public:
        AssetLoaderSystem(AssetLoader& loader);

        void setup(ecs2::SystemScheduler& sched) override;

        template<typename T>
        void register_component_type() {
            _infos << LoadableComponentTypeInfo {
                fmt_to_owned("loading_tag<{}>", ct_type_name<T>()),
                &load_components<T, false>,
                &load_components<T, true>,
                &update_loading_status<T>
            };
        }

    private:
        void run_tick(bool only_recent);

    private:
        struct LoadableComponentTypeInfo {
            core::String loading_tag;
            void (*load_all)(ecs2::EntityWorld&, AssetLoadingContext&, std::string_view tag) = nullptr;
            void (*load_recent)(ecs2::EntityWorld&, AssetLoadingContext&, std::string_view tag) = nullptr;
            void (*update_status)(ecs2::EntityWorld&, std::string_view tag) = nullptr;
        };

        template<typename T, bool Recent>
        static void load_components(ecs2::EntityWorld& world, AssetLoadingContext& loading_ctx, std::string_view tag) {
            auto query = [&] {
                if constexpr(Recent) {
                    return world.create_group<ecs2::Mutate<ecs2::Changed<T>>>().query();
                } else {
                    return world.create_group<ecs2::Mutate<T>>().query();
                }
            }();

            for(auto&& [id, comp] : query.id_components()) {
                comp.load_assets(loading_ctx);
                world.add_tag(id, tag);
            }
        }

        template<typename T>
        static void update_loading_status(ecs2::EntityWorld& world, std::string_view tag) {
            auto query = world.create_group<ecs2::Mutate<T>>(tag).query();
            for(auto&& [id, comp] : query.id_components()) {
                y_debug_assert(world.has_tag(id, tag));
                if(comp.update_asset_loading_status()) {
                    world.remove_tag(id, tag);
                }
            }
        }

        core::Vector<LoadableComponentTypeInfo> _infos;
        AssetLoader* _loader = nullptr;

};

}

#endif // YAVE_SYSTEMS_ASSETLOADERSYSTEM_H

