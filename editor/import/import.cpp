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

#include "import.h"
#include "image_utils.h"

#include <yave/meshes/Vertex.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/MaterialData.h>
#include <yave/utils/FileSystemModel.h>

#include <y/io2/File.h>
#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


// -------------- TINYGLTF --------------
#ifdef Y_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

#ifdef Y_MSVC
#pragma warning(push)
#pragma warning(disable:4018) // signed/unsigned mismatch
#pragma warning(disable:4267) // conversion from 'size_t' to 'int', possible loss of data
#endif


#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NOEXCEPTION
#include <external/tinygltf/tiny_gltf.h>


#ifdef Y_MSVC
#pragma warning(pop)
#endif

#ifdef Y_GNU
#pragma GCC diagnostic pop
#endif

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
        return "unamed";
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

    return n.is_empty() ? core::String("unamed") : n;
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
        switch(bpp) {
            case 3:
                img = compress_bc1(img);
            break;

            /* case 1:
                img = compress_bc4(img);
            break; */

            default:
                log_msg("Compression is not supported for the given image format", Log::Warning);
        }
    }

    return core::Ok(std::move(img));
}

core::String supported_image_extensions() {
    return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}







static core::Result<usize> component_count(int type) {
    switch(type) {
        case TINYGLTF_TYPE_SCALAR:  return core::Ok(1_uu);
        case TINYGLTF_TYPE_VEC2:    return core::Ok(2_uu);
        case TINYGLTF_TYPE_VEC3:    return core::Ok(3_uu);
        case TINYGLTF_TYPE_VEC4:    return core::Ok(4_uu);
        case TINYGLTF_TYPE_MAT2:    return core::Ok(4_uu);
        case TINYGLTF_TYPE_MAT3:    return core::Ok(9_uu);
        case TINYGLTF_TYPE_MAT4:    return core::Ok(16_uu);
        default:
            return core::Err();
    }
}

y_force_inline static math::Vec3 change_base(const math::Vec3& v) {
    return math::Vec3(v.z(), v.x(), v.y());
}

y_force_inline static math::Vec3 change_base_euler(const math::Vec3& v) {
    return math::Vec3(v.z(), v.x(), -v.y());
}

static math::Transform<> parse_node_transform_matrix(const tinygltf::Node& node) {
    y_debug_assert(node.matrix.size() == 16);

    math::Matrix4<> mat;
    std::transform(node.matrix.begin(), node.matrix.end(), mat.begin(), [](double f) {
        return float(f);
    });


    y_debug_assert(math::all_finite(mat));
    y_debug_assert(mat[3][3] == 1.0f);
    return mat;
}

static math::Transform<> parse_node_transform(const tinygltf::Node& node) {
    if(node.matrix.size() == 16) {
        return parse_node_transform_matrix(node);
    }

    y_debug_assert(node.matrix.empty());

    math::Vec3 translation;
    for(usize k = 0; k != node.translation.size(); ++k) {
        translation[k] = float(node.translation[k]);
    }

    math::Vec3 scale(1.0f, 1.0f, 1.0f);
    for(usize k = 0; k != node.scale.size(); ++k) {
        scale[k] = float(node.scale[k]);
    }

    math::Vec4 rotation(0.0f, 0.0f, 0.0f, 1.0f);
    for(usize k = 0; k != node.rotation.size(); ++k) {
        rotation[k] = float(node.rotation[k]);
    }

    const math::Transform<> tr = math::Transform<>(translation, rotation, scale);
    y_debug_assert(math::all_finite(tr));
    y_debug_assert(tr[3][3] == 1.0f);
    return tr;
}

template<typename T>
void propagate_transforms(T& nodes, int index, math::Transform<> parent) {
    auto& node = nodes[index];
    node.transform = parent * node.transform;

    for(const int child_index : node.children) {
        propagate_transforms(nodes, child_index, node.transform);
    }
}

template<typename T, bool Remap = true>
static void read_attrib(T& v, core::Span<typename T::value_type> s) {
    std::copy_n(s.begin(), std::min(v.size(), s.size()), v.begin());
    if constexpr(Remap) {
        v.to<3>() = change_base(v.to<3>());
    }
}

template<usize I, bool Remap = true>
static void read_packed_unit_vec(math::Vec2ui& packed, core::Span<float> s) {
    math::Vec4 v;
    read_attrib<math::Vec4, Remap>(v, s);
    packed[I] = pack_2_10_10_10(v.to<3>().normalized(), v.w() < 0.0f);
}


template<typename T, typename F>
static void import_stream(const tinygltf::Model& model, const tinygltf::Accessor& accessor, core::MutableSpan<T> stream, F&& packer) {
    const usize components = component_count(accessor.type).unwrap();
    const tinygltf::BufferView& buffer = model.bufferViews[accessor.bufferView];

    const u8* begin = model.buffers[buffer.buffer].data.data() + buffer.byteOffset + accessor.byteOffset;
    const usize stride = buffer.byteStride ? buffer.byteStride : (components * sizeof(float));

    for(usize i = 0; i != stream.size(); ++i) {
        const float* attrib_begin = reinterpret_cast<const float*>(begin + (stride * i));
        packer(stream[i], core::Span<float>(attrib_begin, components));
    }
}

static core::Result<MeshVertexStreams> import_vertices(const tinygltf::Model& model, const tinygltf::Primitive& primitive) {
    usize size = 0;
    for(auto [name, id] : primitive.attributes) {
        const usize count = model.accessors[id].count;
        if(!size) {
            size = count;
        } else if(size != count) {
            return core::Err();
        }
    }

    bool has_tangents = false;

    MeshVertexStreams streams(size);
    for(const auto& [name, id] : primitive.attributes) {
        const tinygltf::Accessor accessor = model.accessors[id];

        if(!accessor.count) {
            continue;
        }

        if(accessor.sparse.isSparse || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || component_count(accessor.type).is_error()) {
            log_msg("Unsupported component type", Log::Error);
            return core::Err();
        }

        if(name == "POSITION") {
            import_stream(model, accessor, streams.stream<VertexStreamType::Position>(), read_attrib<math::Vec3>);
        } else if(name == "NORMAL") {
            import_stream(model, accessor, streams.stream<VertexStreamType::NormalTangent>(), read_packed_unit_vec<0>);
        } else if(name == "TANGENT") {
            has_tangents = true;
            import_stream(model, accessor, streams.stream<VertexStreamType::NormalTangent>(), read_packed_unit_vec<1>);
        } else if(name == "TEXCOORD_0") {
            import_stream(model, accessor, streams.stream<VertexStreamType::Uv>(), read_attrib<math::Vec2, false>);
        } else {
            log_msg(fmt("Attribute \"{}\" is not supported", name), Log::Warning);
        }
    }

    if(!has_tangents) {
        log_msg("Mesh doesn't have tangents", Log::Warning);
    }

    return core::Ok(std::move(streams));
}

static core::Result<core::Vector<IndexedTriangle>> import_triangles(const tinygltf::Model& model, const tinygltf::Primitive& primitive) {
    const tinygltf::Accessor accessor = model.accessors[primitive.indices];

    if(!accessor.count || accessor.sparse.isSparse) {
        return core::Err();
    }

    core::Vector<IndexedTriangle> triangles;
    triangles.set_min_size(accessor.count / 3);

    auto decode_indices = [&](u32 elem_size, auto convert_index) {
        const tinygltf::BufferView& buffer = model.bufferViews[accessor.bufferView];
        const u8* begin = model.buffers[buffer.buffer].data.data() + buffer.byteOffset + accessor.byteOffset;
        const usize stride = buffer.byteStride ? buffer.byteStride : elem_size;

        for(usize i = 0; i != accessor.count; ++i) {
            triangles[i / 3][i % 3] = convert_index(begin + (i * stride));
        }
    };

    switch(accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_BYTE:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            decode_indices(1, [](const u8* data) -> u32 { return *data; });
        break;

        case TINYGLTF_PARAMETER_TYPE_SHORT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            decode_indices(2, [](const u8* data) -> u32 { return *reinterpret_cast<const u16*>(data); });
        break;

        case TINYGLTF_PARAMETER_TYPE_INT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            decode_indices(4, [](const u8* data) -> u32 { return *reinterpret_cast<const u32*>(data); });
        break;

        default:
            return core::Err();
    }

    return core::Ok(std::move(triangles));
}



core::Result<ParsedScene> parse_scene(const core::String& filename) {
    y_profile();

    core::DebugTimer timer("Parsing gltf");

    ParsedScene scene;
    scene.filename = filename;
    scene.name = clean_asset_name(filename);
    scene.gltf = decltype(scene.gltf)(new tinygltf::Model(), [](tinygltf::Model* ptr) { delete ptr; });

    {
        y_profile_zone("glTF import");

        tinygltf::TinyGLTF ctx;

        // Set dummy loading function to avoid errors
        ctx.SetImageLoader([](tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void*) { return true; }, nullptr);

        const bool is_ascii = filename.ends_with(".gltf");
        const std::string file = filename.data();

        std::string err;
        std::string warn;
        const bool ok = is_ascii
                ? ctx.LoadASCIIFromFile(scene.gltf.get(), &err, &warn, file)
                : ctx.LoadBinaryFromFile(scene.gltf.get(), &err, &warn, file);

        if(!warn.empty()) {
            log_msg(warn, Log::Warning);
        }

        if(!err.empty()) {
            log_msg(err, Log::Error);
        }

        if(!ok) {
            return core::Err();
        }
    }


    for(const tinygltf::Node& gltf_node : scene.gltf->nodes) {
        auto& node = scene.nodes.emplace_back();

        node.name = gltf_node.name;
        node.transform = parse_node_transform(gltf_node);
        node.mesh_index = gltf_node.mesh;
        node.light_index = gltf_node.light;
        node.children = core::Vector<int>::from_range(gltf_node.children);

        if(node.name.is_empty()) {
            node.name = fmt_to_owned("node_{}", scene.nodes.size());
        }
    }

    for(const tinygltf::Image& gltf_image : scene.gltf->images) {
        auto& texture = scene.images.emplace_back();

        texture.name = gltf_image.name;

        if(texture.name.is_empty()) {
            texture.name = fmt_to_owned("texture_{}", scene.materials.size());
        }
    }

    for(const tinygltf::Material& gltf_material : scene.gltf->materials) {
        auto& material = scene.materials.emplace_back();

        material.name = gltf_material.name;

        if(material.name.is_empty()) {
            material.name = fmt_to_owned("material_{}", scene.materials.size());
        }

        auto compute_image_index = [&](int texture_index) {
            if(texture_index >= 0) {
                if(const int image_index = scene.gltf->textures[texture_index].source; image_index >= 0) {
                    return image_index;
                }
            }
            return -1;
        };


        if(const auto it = gltf_material.extensions.find("KHR_materials_specular"); it != gltf_material.extensions.end() && it->second.IsObject()) {
            if(const auto tex_info = it->second.Get("specularColorTexture"); tex_info.IsObject()) {
                if(const auto value = tex_info.Get("index"); value.IsInt()) {
                    if(const int image_index = value.GetNumberAsInt(); image_index >= 0) {
                        scene.images[image_index].as_sRGB = true;
                    }
                }
            }
        }

        if(const int image_index = compute_image_index(gltf_material.pbrMetallicRoughness.baseColorTexture.index); image_index >= 0) {
            scene.images[image_index].as_sRGB = true;
        }

        if(const int image_index = compute_image_index(gltf_material.normalTexture.index); image_index >= 0) {
            scene.images[image_index].as_normal = true;
        }
    }


    for(const tinygltf::Mesh& gltf_mesh : scene.gltf->meshes) {
        auto& mesh = scene.meshes.emplace_back();

        mesh.name = gltf_mesh.name;

        for(const auto& primitive : gltf_mesh.primitives) {
            mesh.materials << primitive.material;
        }

        if(mesh.name.is_empty()) {
            mesh.name = fmt_to_owned("mesh_{}", scene.meshes.size());
        }
    }


    for(const tinygltf::Light& gltf_light : scene.gltf->lights) {
        auto& light = scene.lights.emplace_back();

        light.name = gltf_light.name;

        if(light.name.is_empty()) {
            light.name = fmt_to_owned("light_{}", scene.lights.size());
        }

        light.intensity = float(gltf_light.intensity);

        for(usize i = 0; i != 3; ++i) {
            light.color[i] = float(gltf_light.color[i]);
        }

        if(gltf_light.range > 0.0) {
            light.range = float(gltf_light.range);
        } else {
            const float intensity = light.color.dot(math::Vec3(light.intensity));
            light.range = std::sqrt(intensity * 100.0f); // Put radius where lum < 1%
        }
    }

    core::Vector<int> root_nodes;
    {
        for(usize i = 0; i != scene.nodes.size(); ++i) {
            for(const int child_index : scene.nodes[i].children) {
                scene.nodes[child_index].parent_index = int(i);
            }
        }

        for(usize i = 0; i != scene.nodes.size(); ++i) {
            if(scene.nodes[i].parent_index < 0) {
                root_nodes << int(i);
                propagate_transforms(scene.nodes, int(i), math::Transform<>());
            }
        }

        for(auto& node : scene.nodes) {
            const auto [t, r, s] = node.transform.decompose();
            node.transform = math::Transform<>(
                change_base(t),
                math::Quaternion<>::from_euler(change_base_euler(r.to_euler())),
                change_base(s)
            );
        }
    }


    if(root_nodes.size() == 1) {
        scene.root_node = root_nodes.first();
    } else {
        scene.root_node = int(scene.nodes.size());
        auto& root = scene.nodes.emplace_back();

        if(root_nodes.is_empty()) {
            for(int i = 0; i != int(scene.nodes.size()); ++i) {
                root.children << i;
            }
        } else {
            root.children = std::move(root_nodes);
        }


        root.name = scene.name;
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

    const tinygltf::Mesh& mesh = gltf->meshes[index];

    MeshData mesh_data;
    for(usize i = 0; i != mesh.primitives.size(); ++i) {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        if(primitive.mode != TINYGLTF_MODE_TRIANGLES) {
            return core::Err();
        }

        auto vertex_streams = import_vertices(*gltf, primitive);
        auto triangles = import_triangles(*gltf, primitive);

        if(vertex_streams.is_error() || triangles.is_error()) {
            return core::Err();
        }

        mesh_data.add_sub_mesh(std::move(vertex_streams.unwrap()), std::move(triangles.unwrap()));
    }

    return core::Ok(std::move(mesh_data));
}

core::Result<MaterialData> ParsedScene::create_material(int index) const {
    if(index < 0) {
        return core::Err();
    }


    const tinygltf::Material& material = gltf->materials[index];
    const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;

    bool use_specular = false;
    int specular_texture_index = -1;
    int specular_color_texture_index = -1;
    float specular_factor = 1.0f;
    math::Vec3 specular_color_factor = math::Vec3(1.0f);

    if(const auto it = material.extensions.find("KHR_materials_specular"); it != material.extensions.end() && it->second.IsObject()) {
        use_specular = true;

        if(const auto tex_info = it->second.Get("specularTexture"); tex_info.IsObject()) {
            if(const auto value = tex_info.Get("index"); value.IsInt()) {
                specular_texture_index = value.GetNumberAsInt();
            }
        }

        if(const auto tex_info = it->second.Get("specularColorTexture"); tex_info.IsObject()) {
            if(const auto value = tex_info.Get("index"); value.IsInt()) {
                specular_color_texture_index = value.GetNumberAsInt();
            }
        }

        if(const auto value = it->second.Get("specularColorFactor"); value.IsArray()) {
            for(int i = 0; i != 3; ++i) {
                specular_color_factor[i] = float(value.Get(i).GetNumberAsDouble());
            }
        }

         if(const auto value = it->second.Get("specularFactor"); value.IsNumber()) {
            specular_factor *= float(value.GetNumberAsDouble());
        }
    }


    auto find_texture = [&](int tex_index) -> AssetPtr<Texture> {
        if(tex_index < 0) {
            return {};
        }

        const int image_index = gltf->textures[tex_index].source;
        if(image_index < 0) {
            return {};
        }

        return make_asset_with_id<Texture>(images[image_index].asset_id);
    };

    auto fill_common = [&](auto& data) {
        data.diffuse = find_texture(pbr.baseColorTexture.index);
        data.normal = find_texture(material.normalTexture.index);
        data.emissive = find_texture(material.emissiveTexture.index);

        for(usize i = 0; i != 3; ++i) {
            data.color_factor[i] = float(material.pbrMetallicRoughness.baseColorFactor[i]);
            data.emissive_factor[i] = float(material.emissiveFactor[i]);
        }

        data.alpha_tested = (material.alphaMode != "OPAQUE");
        data.double_sided = material.doubleSided;
    };

    MaterialData mat_data;

    if(use_specular) {
        MaterialData::SpecularMaterialData data;
        fill_common(data.common);
        data.specular = find_texture(specular_texture_index);
        data.specular_color = find_texture(specular_color_texture_index);
        data.specular_color_factor = specular_color_factor;
        data.specular_factor = specular_factor;

        mat_data = MaterialData(std::move(data));
    } else {
        MaterialData::MetallicRoughnessMaterialData data;
        fill_common(data.common);
        data.metallic_roughness = find_texture(pbr.metallicRoughnessTexture.index);
        data.metallic_factor = float(pbr.metallicFactor);
        data.roughness_factor = float(pbr.roughnessFactor);

        mat_data = MaterialData(std::move(data));
    }


    return core::Ok(std::move(mat_data));
}

core::Result<ImageData> ParsedScene::create_image(int index, bool compress) const {
    if(index < 0) {
        return core::Err();
    }

    ImageImportFlags flags = ImageImportFlags::None;
    if(compress && !images[index].as_normal) {
        flags = flags | ImageImportFlags::Compress;
    }
    if(images[index].as_sRGB) {
        flags = flags | ImageImportFlags::ImportAsSRGB;
    }
    if(images[index].generate_mips) {
        flags = flags | ImageImportFlags::GenerateMipmaps;
    }

    const tinygltf::Image& image = gltf->images[index];

    if(!image.uri.empty()) {
        const FileSystemModel* fs = FileSystemModel::local_filesystem();
        const auto path = fs->parent_path(filename);
        const core::String image_path = path ? fs->join(path.unwrap(), image.uri) : core::String(image.uri);
        return import_image(image_path, flags);
    }

    const tinygltf::BufferView& view = gltf->bufferViews[image.bufferView];
    const tinygltf::Buffer& buffer = gltf->buffers[view.buffer];
    return import_image(core::Span<u8>(buffer.data.data() + view.byteOffset, view.byteLength), flags);
}

core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}

}
}

