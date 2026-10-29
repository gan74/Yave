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
#ifndef EDITOR_IMPORT_IMPORT_H
#define EDITOR_IMPORT_IMPORT_H

#include "gltf.h"

#include <yave/graphics/images/ImageData.h>
#include <yave/assets/AssetId.h>

#include <y/core/Result.h>
#include <y/core/Vector.h>
#include <y/math/math.h>
#include <y/concurrent/JobSystem.h>

#include <atomic>
#include <memory>
#include <utility>


namespace editor {
namespace import {

// ----------------------------- UTILS -----------------------------
core::String clean_asset_name(const core::String& name);





// ----------------------------- SCENE -----------------------------
struct GltfImportSettings {
    core::String import_path;
    bool import_child_prefabs_as_assets = false;
    bool create_colliders = false;
    bool compress_textures = true;
};

struct GltfImportStatus : NonMovable {
    std::atomic<bool> cancelled = false;

    std::atomic<usize> done = 0;
    std::atomic<usize> failed = 0;
    usize total = 0;

    // Only valid once the returned job is finished
    AssetId root_prefab = AssetId::invalid_id();
};
std::pair<concurrent::JobSystem::JobHandle, std::shared_ptr<GltfImportStatus>> schedule_gltf_import(concurrent::JobSystem& job_system, gltf::Scene scene, GltfImportSettings settings);

core::String supported_scene_extensions();





// ----------------------------- IMAGES -----------------------------
enum class ImageImportFlags {
    None = 0x00,

    GenerateMipmaps = 0x01,
    Compress        = 0x02,

    ImportAsSRGB    = 0x04,
    IsNormalMap     = 0x08,
};

core::Result<ImageData> import_image(const core::String& filename, ImageImportFlags flags = ImageImportFlags::None);
core::Result<ImageData> import_image(core::Span<u8> image_data, ImageImportFlags flags = ImageImportFlags::None);
core::String supported_image_extensions();








constexpr ImageImportFlags operator|(ImageImportFlags l, ImageImportFlags r) {
    return ImageImportFlags(uenum(l) | uenum(r));
}

constexpr ImageImportFlags operator&(ImageImportFlags l, ImageImportFlags r)  {
    return ImageImportFlags(uenum(l) & uenum(r));
}

}
}

#endif // EDITOR_IMPORT_IMPORT_H

