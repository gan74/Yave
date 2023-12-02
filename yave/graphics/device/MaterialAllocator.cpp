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

#include "MaterialAllocator.h"

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/material/MaterialData.h>

#include <yave/graphics/device/extensions/DebugUtils.h>

#include <numeric>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static std::array<TextureView, MaterialData::texture_count> material_texture_views(const MaterialData& data) {
    std::array<TextureView, MaterialData::texture_count> textures = {
        *device_resources()[DeviceResources::GreyTexture],          // Diffuse
        *device_resources()[DeviceResources::FlatNormalTexture],    // Normal
        *device_resources()[DeviceResources::WhiteTexture],         // Roughness
        *device_resources()[DeviceResources::WhiteTexture],         // Metallic
        *device_resources()[DeviceResources::WhiteTexture],         // Emissive
    };

    for(usize i = 0; i != MaterialData::texture_count; ++i) {
        y_debug_assert(!data.textures()[i].is_loading());
        if(const auto* tex = data.textures()[i].get()) {
            textures[i] = *tex;
        }
    }

    return textures;
}


MaterialAllocator::MaterialAllocator() : _materials(max_materials) {
    _free.locked([&](auto&& free) {
        free.set_min_size(_materials.size());
        std::iota(free.begin(), free.end(), 0);
    });

#ifdef Y_DEBUG
    if(const auto* debug = debug_utils()) {
        debug->set_resource_name(_materials.vk_buffer(), "Material allocator material buffer");
    }
#endif
}

MaterialAllocator::~MaterialAllocator() {
    _free.locked([&](auto&& free) {
        y_always_assert(free.size() == _materials.size(), "Not all materials have been released");
    });
}

MaterialDrawData MaterialAllocator::allocate_material(const MaterialData& material) {
    const auto textures = material_texture_views(material);

    uniform::MaterialData data = {};
    {
        data.emissive_mul = material.emissive_mul();
        data.base_color_mul = material.base_color_mul();
        data.roughness_mul = material.roughness_mul();
        data.metallic_mul = material.metallic_mul();
        y_debug_assert(textures.size() <= data.texture_count);
        for(usize i = 0; i != textures.size(); ++i) {
            data.texture_indices[i] = texture_library().add_texture(textures[i]);
        }
    }

    const u32 index = _free.locked([&](auto&& free) {
        y_always_assert(!free.is_empty(), "Max number of materials reached");
        return free.pop();
    });

    {
        Y_TODO(does map need to be synchronized?)
        auto mapping = _materials.map(MappingAccess::ReadWrite);
        mapping[index] = data;
    }

    MaterialDrawData draw_data;
    draw_data._index = index;
    draw_data._textures = textures;
    draw_data._parent = this;
    return draw_data;
}

void MaterialAllocator::recycle(MaterialDrawData* data) {
    y_debug_assert(!data->is_null());
    y_debug_assert(data->_parent == this);

    _free.locked([&](auto&& free) {
        free << data->_index;
    });

    for(const auto& tex : data->_textures) {
        texture_library().remove_texture(tex);
    }

    data->_index = u32(-1);
    data->_parent = nullptr;
    y_debug_assert(data->is_null());
}

}
