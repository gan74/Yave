/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_COMPONENTS_SKYLIGHTCOMPONENT_H
#define YAVE_COMPONENTS_SKYLIGHTCOMPONENT_H

#include <yave/assets/AssetPtr.h>
#include <yave/graphics/images/IBLProbe.h>
#include <yave/systems/AssetLoaderSystem.h>

namespace yave {

class SkyLightComponent final : public ecs::RegisterComponent<SkyLightComponent, AssetLoaderSystem> {
    public:
        AssetPtr<IBLProbe>& probe();
        const AssetPtr<IBLProbe>& probe() const;

        bool& display_sky();
        bool display_sky() const;

        float& intensity();
        float intensity() const;

        bool update_asset_loading_status();
        void load_assets(AssetLoadingContext& loading_ctx);

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(SkyLightComponent, _probe, _intensity, _display_sky)

    private:
        AssetPtr<IBLProbe> _probe;
        float _intensity = 1.0f;
        bool _display_sky = false;
};
}

#endif // YAVE_COMPONENTS_SKYLIGHTCOMPONENT_H

