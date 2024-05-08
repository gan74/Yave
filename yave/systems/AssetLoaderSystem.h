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

#include <y/utils/format.h>

namespace yave {

class AssetLoaderSystem : public ecs::System {
    public:
        AssetLoaderSystem(AssetLoader& loader);

        void setup(ecs::SystemScheduler& sched) override;

        template<typename T>
        void register_component_type() {
            _infos << LoadableComponentTypeInfo {
                ct_type_name<T>(),
                fmt_to_owned("loading_tag<{}>", ct_type_name<T>()),
                &load_components<T, false>,
                &load_components<T, true>,
                &update_loading_status<T>
            };
        }

    private:
        struct LoadableComponentTypeInfo {
            std::string_view type_name;
            core::String loading_tag;
            void (*load_all)(ecs::EntityWorld&, AssetLoadingContext&, const core::String& tag) = nullptr;
            void (*load_recent)(ecs::EntityWorld&, AssetLoadingContext&, const core::String& tag) = nullptr;
            void (*update_status)(ecs::EntityWorld&, const core::String& tag) = nullptr;
        };

        template<typename T, bool Recent>
        static void load_components(ecs::EntityWorld& world, AssetLoadingContext& loading_ctx, const core::String& tag) {
            auto query = [&] {
                if constexpr(Recent) {
                    return world.create_group<ecs::Mutate<ecs::Changed<T>>>().query();
                } else {
                    return world.create_group<ecs::Mutate<T>>().query();
                }
            }();

            y_profile_msg(fmt_c_str("Processing {} components", query.size()));
            for(auto&& [id, comp] : query.id_components()) {
                comp.load_assets(loading_ctx);
                world.add_tag(id, tag);
            }
        }

        template<typename T>
        static void update_loading_status(ecs::EntityWorld& world, const core::String& tag) {
            auto query = world.create_group<ecs::Mutate<T>>(tag.view()).query();
            for(auto&& [id, comp] : query.id_components()) {
                y_debug_assert(world.has_tag(id, tag));
                if(comp.update_asset_loading_status()) {
                    Y_TODO(this is not thread safe!!!)
                    world.remove_tag(id, tag);
                    y_debug_assert(!world.has_tag(id, tag));
                }
            }
        }

        core::Vector<LoadableComponentTypeInfo> _infos;
        AssetLoader* _loader = nullptr;

};

}

#endif // YAVE_SYSTEMS_ASSETLOADERSYSTEM_H

