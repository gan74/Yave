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

#include "SkyLightComponent.h"

#include <yave/graphics/images/ImageData.h>
#include <yave/assets/AssetLoader.h>

#include <yave/ecs/ComponentInspector.h>

namespace yave {

AssetPtr<IBLProbe>& SkyLightComponent::probe() {
    return _probe;
}

const AssetPtr<IBLProbe>& SkyLightComponent::probe() const {
    return _probe;
}

bool& SkyLightComponent::display_sky() {
    return _display_sky;
}

bool SkyLightComponent::display_sky() const {
    return _display_sky;
}

float& SkyLightComponent::intensity() {
    return _intensity;
}

float SkyLightComponent::intensity() const {
    return _intensity;
}

bool SkyLightComponent::update_asset_loading_status() {
    return !_probe.is_loading();
}

void SkyLightComponent::load_assets(AssetLoadingContext& loading_ctx) {
    _probe.load(loading_ctx);
}

void SkyLightComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Envmap", _probe);
    inspector->inspect("Intensity", _intensity, 0.0f);
    inspector->inspect("Display sky", _display_sky);
}


}

