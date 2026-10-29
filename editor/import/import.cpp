/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining as_alb copy
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

#include "import.h"
#include "image_utils.h"

#include <yave/meshes/Vertex.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/MaterialData.h>

#include <y/io2/File.h>
#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


// -------------- STB --------------
extern "C" {
#ifdef Y_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <external/stb/stb_image.h>

#ifdef Y_GNU
#pragma GCC diagnostic pop
#endif
}


namespace editor {
namespace import {

core::String clean_asset_name(const core::String& name) {
    if(name.is_empty()) {
        return "unnamed";
    }

    usize begin = 0;
    for(usize i = 0; i != name.size(); ++i) {
        if(name[i] == '\\' || name[i] == '/') {
            begin = i + 1;
        }
    }

    usize len = 0;
    for(usize i = begin; i != name.size(); ++i) {
        if(name[i] == '.') {
            break;
        }
        ++len;
    }

    core::String n = name.sub_str(begin, len);
    for(char& c : n) {
        if(!std::isalnum(c)) {
            c = '_';
        }
    }

    return n.is_empty() ? core::String("unnamed") : n;
}

core::Result<ImageData> import_image(const core::String& filename, ImageImportFlags flags) {
    if(auto file = io2::File::open(filename)) {
        core::Vector<u8> data;
        if(!file.unwrap().read_all(data)) {
            log_msg(fmt_c_str("Unable to read image \"{}\"", filename), Log::Error);
            return core::Err();
        }

        return import_image(data, flags);
    }

    log_msg(fmt_c_str("Unable to open image \"{}\"", filename), Log::Error);
    return core::Err();
}

core::Result<ImageData> import_image(core::Span<u8> image_data, ImageImportFlags flags) {
    y_profile();

    const usize req_components = 4;

    int width, height, bpp;
    u8* stbi_data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(image_data.data()), int(image_data.size()), &width, &height, &bpp, req_components);
    y_defer(stbi_image_free(stbi_data));

    if(!stbi_data || width < 1 || height < 1) {
        log_msg("Unable to load image", Log::Error);
        return core::Err();
    }

    ImageFormat format;
    if((flags & ImageImportFlags::ImportAsSRGB) == ImageImportFlags::ImportAsSRGB) {
        format = VK_FORMAT_R8G8B8A8_SRGB;
    } else {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    if(!format.is_valid()) {
        log_msg("Invalid image format", Log::Error);
        return core::Err();
    }

    ImageData img(math::Vec2ui(width, height), stbi_data, format);
    if((flags & ImageImportFlags::GenerateMipmaps) == ImageImportFlags::GenerateMipmaps) {
        img = compute_mipmaps(img);
    }

    if((flags & ImageImportFlags::Compress) == ImageImportFlags::Compress) {
        const bool normal = (flags & ImageImportFlags::IsNormalMap) == ImageImportFlags::IsNormalMap;
        img = compress(img, normal ? ImageCompression::BC5 : ImageCompression::BC7);
    }

    return core::Ok(std::move(img));
}

core::String supported_image_extensions() {
    return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}







template<typename T>
static void propagate_transforms(T& nodes, int index, math::Transform<> parent) {
    auto& node = nodes[index];
    node.transform = parent * node.transform;

    for(const int child_index : node.children) {
        propagate_transforms(nodes, child_index, node.transform);
    }
}

core::Result<ParsedScene> parse_scene(const core::String& filename) {
    y_profile();

    core::DebugTimer timer("Parsing gltf");

    auto loaded = gltf::load_gltf(filename);
    if(!loaded) {
        return core::Err();
    }

    ParsedScene scene;
    scene.filename = filename;
    scene.name = clean_asset_name(filename);
    scene.source = std::move(loaded.unwrap());

    const gltf::Scene& source = scene.source;

    for(const gltf::Image& src_image : source.images) {
        auto& image = scene.images.emplace_back();
        image.name = src_image.name;
        image.as_sRGB = src_image.color_space == gltf::ColorSpace::sRGB;
        image.as_normal = src_image.is_normal_map;
    }

    for(const gltf::Material& src_material : source.materials) {
        auto& material = scene.materials.emplace_back();
        material.name = src_material.name;
    }

    for(const gltf::Mesh& src_mesh : source.meshes) {
        auto& mesh = scene.meshes.emplace_back();
        mesh.name = src_mesh.name;

        for(const gltf::Primitive& primitive : src_mesh.primitives) {
            mesh.materials << primitive.material;
        }
    }

    for(const gltf::Light& src_light : source.lights) {
        auto& light = scene.lights.emplace_back();

        light.name = src_light.name;
        light.intensity = src_light.intensity;
        light.color = src_light.color;

        if(src_light.range > 0.0f) {
            light.range = src_light.range;
        } else {
            const float intensity = light.color.dot(math::Vec3(light.intensity));
            light.range = std::sqrt(intensity * 100.0f); // Put radius where lum < 1%
        }
    }

    for(const gltf::Node& src_node : source.nodes) {
        auto& node = scene.nodes.emplace_back();

        node.name = src_node.name;
        node.transform = src_node.local_transform;
        node.mesh_index = src_node.mesh;
        node.light_index = src_node.light;
        node.parent_index = src_node.parent;
        node.children = core::Vector<int>(src_node.children);
    }

    for(const int root : source.roots) {
        propagate_transforms(scene.nodes, root, math::Transform<>());
    }

    if(source.roots.size() == 1) {
        scene.root_node = source.roots.first();
    } else {
        scene.root_node = int(scene.nodes.size());
        auto& root = scene.nodes.emplace_back();

        root.name = scene.name;
        root.children = core::Vector<int>(source.roots);
        for(int child : root.children) {
            scene.nodes[child].parent_index = scene.root_node;
        }
    }

    return core::Ok(std::move(scene));
}

core::Result<MeshData> ParsedScene::create_mesh(int index) const {
    if(index < 0) {
        return core::Err();
    }

    const gltf::Mesh& mesh = source.meshes[index];
    if(mesh.primitives.is_empty()) {
        log_msg(fmt("Mesh \"{}\" has no valid primitive", mesh.name), Log::Error);
        return core::Err();
    }

    MeshData mesh_data;
    for(const gltf::Primitive& primitive : mesh.primitives) {
        mesh_data.add_sub_mesh(primitive.vertices, primitive.triangles);
    }

    return core::Ok(std::move(mesh_data));
}

core::Result<MaterialData> ParsedScene::create_material(int index) const {
    if(index < 0) {
        return core::Err();
    }

    const gltf::Material& material = source.materials[index];

    auto find_texture = [&](const gltf::TextureRef& ref) -> AssetPtr<Texture> {
        if(!ref.is_valid()) {
            return {};
        }
        return make_asset_with_id<Texture>(images[ref.image].asset_id);
    };

    auto fill_common = [&](auto& data) {
        data.diffuse = find_texture(material.base_color);
        data.normal = find_texture(material.normal);
        data.emissive = find_texture(material.emissive);

        data.color_factor = material.base_color_factor.to<3>();
        data.emissive_factor = material.emissive_factor;

        data.alpha_tested = (material.alpha_mode == gltf::AlphaMode::Mask);
        data.blend_mode = (material.alpha_mode == gltf::AlphaMode::Blend ? BlendMode::SrcAlpha : BlendMode::None);
        data.double_sided = material.double_sided;
    };

    MaterialData mat_data;

    if(material.has_specular) {
        MaterialData::SpecularMaterialData data;
        fill_common(data.common);
        data.specular = find_texture(material.specular);
        data.specular_color = find_texture(material.specular_color);
        data.specular_color_factor = material.specular_color_factor;
        data.specular_factor = material.specular_factor;

        mat_data = MaterialData(std::move(data));
    } else {
        MaterialData::MetallicRoughnessMaterialData data;
        fill_common(data.common);
        data.metallic_roughness = find_texture(material.metallic_roughness);
        data.metallic_factor = material.metallic_factor;
        data.roughness_factor = material.roughness_factor;

        mat_data = MaterialData(std::move(data));
    }

    return core::Ok(std::move(mat_data));
}

core::Result<ImageData> ParsedScene::create_image(int index, bool compress) const {
    if(index < 0) {
        return core::Err();
    }

    ImageImportFlags flags = ImageImportFlags::None;
    if(compress) {
        flags = flags | ImageImportFlags::Compress;
    }

    if(images[index].as_sRGB && !images[index].as_normal) {
        flags = flags | ImageImportFlags::ImportAsSRGB;
    }

    if(!images[index].as_sRGB && images[index].as_normal) {
        flags = flags | ImageImportFlags::IsNormalMap;
    }

    if(images[index].generate_mips) {
        flags = flags | ImageImportFlags::GenerateMipmaps;
    }

    const gltf::Image& image = source.images[index];

    if(!image.path.is_empty()) {
        return import_image(image.path, flags);
    }

    if(image.data.is_empty()) {
        log_msg(fmt("Image \"{}\" has no data", image.name), Log::Error);
        return core::Err();
    }

    return import_image(image.data, flags);
}

core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}

}
}

