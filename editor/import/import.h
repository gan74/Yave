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
#ifndef EDITOR_IMPORT_IMPORT_H
#define EDITOR_IMPORT_IMPORT_H

#include <yave/graphics/images/ImageData.h>
#include <yave/meshes/MeshData.h>
#include <yave/animations/Animation.h>
#include <yave/material/MaterialData.h>

#include <y/core/Result.h>
#include <y/core/Vector.h>
#include <y/math/math.h>


namespace tinygltf {
class Model;
}

namespace editor {
namespace import {

// ----------------------------- UTILS -----------------------------
core::String clean_asset_name(const core::String& name);





// ----------------------------- SCENE -----------------------------
struct ParsedScene {
    struct Asset {
        bool is_error = false;

        core::String name = "unamed asset";

        int gltf_index = -1;
        AssetId asset_id;
    };

    struct MaterialGroup {
        int primitive_index = -1;
        int gltf_material_index = -1;
    };

    struct MeshPrefab : Asset {
        AssetId mesh_asset_id;
        core::Vector<MaterialGroup> materials;
    };

    struct Image : Asset {
       bool as_sRGB = false;
       bool generate_mips = true;
    };

    struct Material : Asset {
    };

    struct Node {
        core::String name = "unamed node";
        math::Transform<> transform;
        int mesh_gltf_index = -1;
    };

    bool is_error = true;

    core::String name;
    core::String filename;

    core::Vector<MeshPrefab> mesh_prefabs;
    core::Vector<Material> materials;
    core::Vector<Image> images;

    core::Vector<Node> nodes;

    std::unique_ptr<tinygltf::Model, std::function<void(tinygltf::Model*)>> gltf;

    core::Result<MeshData> build_mesh_data(usize index) const;
    core::Result<ImageData> build_image_data(usize index, bool compress = true) const;
    core::Result<MaterialData> build_material_data(usize index) const;
};


ParsedScene parse_scene(const core::String& filename);
core::String supported_scene_extensions();





// ----------------------------- IMAGES -----------------------------
enum class ImageImportFlags {
    None = 0x00,

    GenerateMipmaps = 0x01,
    ImportAsSRGB    = 0x02,
    Compress        = 0x04,
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

