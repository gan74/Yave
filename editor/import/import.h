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
#ifndef EDITOR_IMPORT_IMPORT_H
#define EDITOR_IMPORT_IMPORT_H

#include <editor/utils/Named.h>

#include <yave/graphics/images/ImageData.h>
#include <yave/meshes/MeshData.h>
#include <yave/animations/Animation.h>
#include <yave/material/SimpleMaterialData.h>

#include <y/core/Vector.h>
#include <y/math/math.h>


namespace editor {
namespace import {

struct SkeletonData {
    core::Vector<SkinWeights> skin;
    core::Vector<Bone> bones;
};

struct MaterialData {
    std::array<core::String, SimpleMaterialData::texture_count> textures;
    float metallic = 0.0f;
    float roughness = 1.0f;
    math::Vec3 emissive;
};

struct SubMeshData {
    core::String mesh;
    core::String material;
};

struct PrefabData {
    core::Vector<SubMeshData> sub_meshes;
};

struct SceneData {
    core::Vector<Named<MeshData>> meshes;
    core::Vector<Named<Animation>> animations;
    core::Vector<Named<ImageData>> images;
    core::Vector<Named<MaterialData>> materials;
    core::Vector<Named<PrefabData>> prefabs;
};


// ----------------------------- UTILS -----------------------------
core::String clean_asset_name(const core::String& name);





// ----------------------------- SCENE -----------------------------
enum class SceneImportFlags {
    None = 0x00,

    ImportMeshes        = 0x01,
    ImportAnims         = 0x02,
    ImportImages        = 0x04,
    ImportMaterials     = 0x08 | ImportImages,
    ImportPrefabs       = 0x10 | ImportMeshes | ImportMaterials,

    FlipUVs             = 0x20,
    ImportDiffuseAsSRGB = 0x40,

    ImportAll = ImportMeshes | ImportAnims | ImportImages | ImportMaterials | ImportPrefabs | ImportDiffuseAsSRGB,

};

SceneData import_scene(const core::String& filename, SceneImportFlags flags = SceneImportFlags::ImportAll);
core::String supported_scene_extensions();





// ----------------------------- IMAGES -----------------------------
enum class ImageImportFlags {
    None = 0x00,

    GenerateMipmaps = 0x01,
    ImportAsSRGB    = 0x02,
};

Named<ImageData> import_image(const core::String& filename, ImageImportFlags flags = ImageImportFlags::None);
core::String supported_image_extensions();








constexpr SceneImportFlags operator|(SceneImportFlags l, SceneImportFlags r) {
    return SceneImportFlags(uenum(l) | uenum(r));
}

constexpr SceneImportFlags operator&(SceneImportFlags l, SceneImportFlags r)  {
    return SceneImportFlags(uenum(l) & uenum(r));
}

constexpr ImageImportFlags operator|(ImageImportFlags l, ImageImportFlags r) {
    return ImageImportFlags(uenum(l) | uenum(r));
}

constexpr ImageImportFlags operator&(ImageImportFlags l, ImageImportFlags r)  {
    return ImageImportFlags(uenum(l) & uenum(r));
}

}
}

#endif // EDITOR_IMPORT_IMPORT_H

