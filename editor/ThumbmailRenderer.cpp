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

#include "ThumbmailRenderer.h"

#include <yave/assets/AssetLoader.h>
#include <yave/material/Material.h>
#include <yave/material/SimpleMaterialData.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/ecs/EntityPrefab.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/descriptors/DescriptorSet.h>

#include <y/serde3/archives.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

static Texture render_texture(const AssetPtr<Texture>& tex) {
    y_profile();

    DevicePtr dptr = tex->device();
    CmdBufferRecorder recorder = create_disposable_cmd_buffer(dptr);
    StorageTexture out(dptr, ImageFormat(VK_FORMAT_R8G8B8A8_UNORM), math::Vec2ui(ThumbmailRenderer::thumbmail_size));
    {
        const DescriptorSet set(dptr, {Descriptor(*tex, SamplerType::LinearClamp), Descriptor(StorageView(out))});
        recorder.dispatch_size(device_resources(dptr)[DeviceResources::CopyProgram],  out.size(), {set});
    }
    std::move(recorder).submit<SyncPolicy::Sync>();
    return out;
}

ThumbmailRenderer::ThumbmailRenderer(AssetLoader& loader) : DeviceLinked(loader.device()), _loader(&loader) {
}

const TextureView* ThumbmailRenderer::thumbmail(AssetId id) {
    if(id == AssetId::invalid_id()) {
        return nullptr;
    }

    const auto lock = y_profile_unique_lock(_lock);

    auto& data = _thumbmails[id];

    if(!data) {
        data = std::make_unique<ThumbmailData>();
        query(id, *data);
        return nullptr;
    }

    if(data->failed) {
        return nullptr;
    } else if(data->view.device()) {
        return &data->view;
    } else if(data->asset_ptr.is_loaded()) {
        data->texture = data->render();
        data->render = []() -> Texture { y_fatal("Thumbmail already rendered"); };
        if(!(data->failed = data->texture.is_null())) {
            data->view = data->texture;
        }
    }

    return nullptr;
}


void ThumbmailRenderer::query(AssetId id, ThumbmailData& data) {
    const AssetType asset_type = _loader->store().asset_type(id).unwrap_or(AssetType::Unknown);

    switch(asset_type) {
        case AssetType::Mesh: {
            auto ptr = _loader->load_async<StaticMesh>(id);
            data.asset_ptr = ptr;
            data.render = []{ log_msg("Mesh loaded"); return Texture(); };
        } break;

        case AssetType::Image: {
            auto ptr = _loader->load_async<Texture>(id);
            data.asset_ptr = ptr;
            data.render = [ptr]{ return render_texture(ptr); };
        } break;

        case AssetType::Material: {
            auto ptr = _loader->load_async<Material>(id);
            data.asset_ptr = ptr;
            data.render = []{ log_msg("Material loaded"); return Texture(); };
        } break;

        case AssetType::Prefab: {
            auto ptr = _loader->load_async<ecs::EntityPrefab>(id);
            data.asset_ptr = ptr;
            data.render = []{ log_msg("Prefab loaded"); return Texture(); };
        } break;

        default:
            data.failed = true;
            log_msg(fmt("Unknown asset type % for %.", asset_type, id.id()), Log::Error);
        break;
    }
}

}
