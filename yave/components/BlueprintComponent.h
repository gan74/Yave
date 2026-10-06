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
#ifndef YAVE_COMPONENTS_BLUEPRINTCOMPONENT_H
#define YAVE_COMPONENTS_BLUEPRINTCOMPONENT_H

#include <yave/assets/AssetPtr.h>
#include <yave/blueprints/Blueprint.h>
#include <yave/systems/AssetLoaderSystem.h>
#include <yave/systems/TriggerSystem.h>

#include <memory>

namespace yave {

class BlueprintComponent final : public ecs::RegisterComponent<BlueprintComponent, AssetLoaderSystem, TriggerSystem> {
    public:
        BlueprintComponent() = default;
        BlueprintComponent(const AssetPtr<Blueprint>& blueprint);

        const AssetPtr<Blueprint>& blueprint() const;

        const BlueprintInstance* instance() const;

        bool update_asset_loading_status();
        void load_assets(AssetLoadingContext& loading_ctx);

        void subscribe_triggers(ecs::TriggerSubscriber& subscriber) const;
        void on_trigger(ecs::EntityWorld& world, ecs::EntityId id, ecs::TriggerTypeIndex type, const void* payload) const;

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(BlueprintComponent, _blueprint)

    private:
        AssetPtr<Blueprint> _blueprint;

        mutable std::shared_ptr<BlueprintInstance> _instance;
};

}

#endif // YAVE_COMPONENTS_BLUEPRINTCOMPONENT_H

