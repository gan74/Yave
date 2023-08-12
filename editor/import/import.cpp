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

#include "import.h"
#include "image_utils.h"
#include "mesh_utils.h"

#include <yave/meshes/Vertex.h>
#include <yave/animations/Animation.h>
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

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NOEXCEPTION
#include <external/tinygltf/tiny_gltf.h>

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
        core::Vector<byte> data;
        if(!file.unwrap().read_all(data)) {
            log_msg(fmt_c_str("Unable to read image \"%\".", filename), Log::Error);
            return core::Err();
        }

        return import_image(data, flags);
    }

    log_msg(fmt_c_str("Unable to open image \"%\".", filename), Log::Error);
    return core::Err();
}

core::Result<ImageData> import_image(core::Span<byte> image_data, ImageImportFlags flags) {
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
                log_msg("Compression is not supported for the given image format", Log::Error);
        }
    }

    return core::Ok(std::move(img));
}

core::String supported_image_extensions() {
    return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}








static usize component_count(int type) {
    switch(type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        case TINYGLTF_TYPE_MAT2: return 4;
        case TINYGLTF_TYPE_MAT3: return 9;
        case TINYGLTF_TYPE_MAT4: return 16;
        default:
            y_throw_msg("Sparse accessors are not supported");
    }
}

template<typename T>
static void decode_attrib_buffer_convert_internal(const tinygltf::Model& model, const tinygltf::BufferView& buffer, const tinygltf::Accessor& accessor, T* vertex_elems, usize vertex_count) {
    using value_type = typename T::value_type;
    static constexpr usize size = T::size();

    const usize components = component_count(accessor.type);
    const bool normalize = accessor.normalized;

    y_debug_assert(accessor.count == vertex_count);

    if(components != size) {
        log_msg(fmt("Expected VEC% attribute, got VEC%", size, components), Log::Warning);
    }

    const usize min_size = std::min(size, components);
    auto convert = [=](const u8* data) {
        T vec;
        for(usize i = 0; i != min_size; ++i) {
            vec[i] = reinterpret_cast<const value_type*>(data)[i];
        }
        if(normalize) {
            if constexpr(size == 4) {
                vec.template to<3>().normalize();
            } else {
                vec.normalize();
            }
        }
        return vec;
    };

    {
        u8* out_begin = reinterpret_cast<u8*>(vertex_elems);

        const auto& in_buffer = model.buffers[buffer.buffer].data;
        const u8* in_begin = in_buffer.data() + buffer.byteOffset + accessor.byteOffset;
        const usize attrib_size = components * sizeof(value_type);
        const usize input_stride = buffer.byteStride ? buffer.byteStride : attrib_size;

        for(usize i = 0; i != accessor.count; ++i) {
            const u8* attrib = in_begin + i * input_stride;
            y_debug_assert(attrib < in_buffer.data() + in_buffer.size());
            *reinterpret_cast<T*>(out_begin + i * sizeof(FullVertex)) = convert(attrib);
        }
    }
}

static void decode_attrib_buffer(const tinygltf::Model& model, const std::string& name, const tinygltf::Accessor& accessor, core::MutableSpan<FullVertex> vertices) {
    const tinygltf::BufferView& buffer = model.bufferViews[accessor.bufferView];

    if(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        y_throw_msg(fmt_c_str("Unsupported component type (%) for \"%\"", accessor.componentType, std::string_view(name)));
    }

    if(name == "POSITION") {
        decode_attrib_buffer_convert_internal(model, buffer, accessor, &vertices[0].position, vertices.size());
    } else if(name == "NORMAL") {
        decode_attrib_buffer_convert_internal(model, buffer, accessor, &vertices[0].normal, vertices.size());
    } else if(name == "TANGENT") {
        decode_attrib_buffer_convert_internal(model, buffer, accessor, &vertices[0].tangent, vertices.size());
    } else if(name == "TEXCOORD_0") {
        decode_attrib_buffer_convert_internal(model, buffer, accessor, &vertices[0].uv, vertices.size());
    } else {
        log_msg(fmt("Attribute \"%\" is not supported", std::string_view(name)), Log::Warning);
        return;
    }
}

static core::Vector<FullVertex> import_vertices(const tinygltf::Model& model, const tinygltf::Primitive& prim) {
    core::Vector<FullVertex> vertices;
    for(auto [name, id] : prim.attributes) {
        tinygltf::Accessor accessor = model.accessors[id];
        if(!accessor.count) {
            continue;
        }

        if(accessor.sparse.isSparse) {
            y_throw_msg("Sparse accessors are not supported");
        }

        if(!vertices.size()) {
            std::fill_n(std::back_inserter(vertices), accessor.count, FullVertex());
        } else if(vertices.size() != accessor.count) {
            y_throw_msg("Invalid attribute count");
        }

        decode_attrib_buffer(model, name, accessor, vertices);
    }
    return vertices;
}

template<typename F>
static void decode_index_buffer(const tinygltf::Model& model, const tinygltf::BufferView& buffer, const tinygltf::Accessor& accessor, IndexedTriangle* triangles, usize elem_size, F convert_index) {
    u32* out_buffer = reinterpret_cast<u32*>(triangles);

    const u8* in_buffer = model.buffers[buffer.buffer].data.data() + buffer.byteOffset + accessor.byteOffset;
    const usize input_stride = buffer.byteStride ? buffer.byteStride : elem_size;

    for(usize i = 0; i != accessor.count; ++i) {
        out_buffer[i] = convert_index(in_buffer + i * input_stride);
    }
}

static core::Vector<IndexedTriangle> import_triangles(const tinygltf::Model& model, const tinygltf::Primitive& prim) {
    tinygltf::Accessor accessor = model.accessors[prim.indices];

    if(!accessor.count) {
        y_throw_msg("Non indexed primitives are not supported");
    }

    if(accessor.sparse.isSparse) {
        y_throw_msg("Sparse accessors are not supported");
    }

    core::Vector<IndexedTriangle> triangles;
    std::fill_n(std::back_inserter(triangles), accessor.count / 3, IndexedTriangle{});
    const tinygltf::BufferView& buffer = model.bufferViews[accessor.bufferView];
    switch(accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_BYTE:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            decode_index_buffer(model, buffer, accessor, triangles.data(), 1, [](const u8* data) -> u32 { return *data; });
        break;

        case TINYGLTF_PARAMETER_TYPE_SHORT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            decode_index_buffer(model, buffer, accessor, triangles.data(), 2, [](const u8* data) -> u32 { return *reinterpret_cast<const u16*>(data); });
        break;

        case TINYGLTF_PARAMETER_TYPE_INT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            decode_index_buffer(model, buffer, accessor, triangles.data(), 4, [](const u8* data) -> u32 { return *reinterpret_cast<const u32*>(data); });
        break;

        default:
            y_throw_msg("Index component type not supported");
    }

    return triangles;
}

static math::Transform<> build_base_change_transform() {
    const math::Vec3 forward = math::Vec3(1.0f, 0.0f, 0.0f);
    const math::Vec3 right = math::Vec3(0.0f, 0.0f, 1.0f);
    const math::Vec3 up = math::Vec3(0.0f, 1.0f, 0.0f);

    math::Transform<> transform;
    transform.set_basis(forward, right, up);
    return transform.inverse();
}

static const math::Transform<> base_change_transform = build_base_change_transform();

static math::Transform<> parse_node_transform(const tinygltf::Node& node) {
    math::Vec3 translation;
    for(usize k = 0; k != node.translation.size(); ++k) {
        translation[k] = float(node.translation[k]);
    }

    math::Vec3 scale(1.0f, 1.0f, 1.0f);
    for(usize k = 0; k != node.scale.size(); ++k) {
        scale[k] = float(node.scale[k]);
    }

    math::Vec4 rotation;
    for(usize k = 0; k != node.rotation.size(); ++k) {
        rotation[k] = float(node.rotation[k]);
    }

    rotation.to<3>() = base_change_transform.transform_direction(rotation.to<3>());
    translation = base_change_transform.transform_direction(translation);
    scale = base_change_transform.transform_direction(scale);

    const math::Transform<> tr = math::Transform<>(translation, rotation, scale);
    y_debug_assert(math::fully_finite(tr));
    y_debug_assert(tr[3][3] == 1.0f);
    return tr;
}

static ParsedScene::Node& parse_node(usize index, ParsedScene& scene) {
    const auto& node = scene.gltf->nodes[index];

    const math::Transform<> transform = parse_node_transform(node);

    const usize node_index = scene.nodes.size();
    scene.nodes.emplace_back(
        node.name.empty() ? fmt_to_owned("unnamed_node_%", index) : clean_asset_name(node.name),
        transform,
        node.mesh
    );

    for(auto& child : node.children) {
        auto& parsed_child = parse_node(child, scene);
        parsed_child.transform = transform * parsed_child.transform;
    }

    return scene.nodes[node_index];
}


ParsedScene parse_scene(const core::String& filename) {
    y_profile();

    core::DebugTimer timer("Parsing gltf");

    ParsedScene scene;
    scene.is_error = false;
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
            return ParsedScene();
        }
    }

    for(int i = 0; i != scene.gltf->meshes.size(); ++i) {
        const tinygltf::Mesh& mesh = scene.gltf->meshes[i];

        auto& parsed_mesh = scene.mesh_prefabs.emplace_back();
        parsed_mesh.name = mesh.name.empty() ? fmt_to_owned("unnamed_mesh_%", i) : clean_asset_name(mesh.name);
        parsed_mesh.gltf_index = i;

        for(int j = 0; j != mesh.primitives.size(); ++j) {
            const tinygltf::Primitive& prim = mesh.primitives[j];

            if(prim.mode != TINYGLTF_MODE_TRIANGLES) {
                continue;
            }

            auto& group = parsed_mesh.materials.emplace_back();
            group.primitive_index = j;
            group.gltf_material_index = prim.material;

        }
    }

    for(int i = 0; i != scene.gltf->images.size(); ++i) {
        const tinygltf::Image& image = scene.gltf->images[i];

        auto& parsed_image = scene.images.emplace_back();
        parsed_image.name = image.name.empty() ? fmt_to_owned("unnamed_image_%", i) : clean_asset_name(image.name);
        parsed_image.gltf_index = i;
    }

    for(int i = 0; i != scene.gltf->materials.size(); ++i) {
        const tinygltf::Material& material = scene.gltf->materials[i];

        auto& parsed_material = scene.materials.emplace_back();
        parsed_material.name = material.name.empty() ? fmt_to_owned("unnamed_material_%", i) : clean_asset_name(material.name);
        parsed_material.gltf_index = i;

        const int tex_index = material.pbrMetallicRoughness.baseColorTexture.index;
        if(tex_index >= 0) {
            const int image_index = scene.gltf->textures[tex_index].source;
            scene.images[image_index].as_sRGB = true;
        }
    }


    if(scene.gltf->defaultScene >= 0) {
        const auto& gltf_scene = scene.gltf->scenes[scene.gltf->defaultScene];
        for(int node : gltf_scene.nodes) {
            parse_node(node, scene);
        }
    }

    return scene;
}

core::Result<MeshData> ParsedScene::build_mesh_data(usize index) const {
    y_profile();

    const MeshPrefab& parsed_mesh = mesh_prefabs[index];
    y_debug_assert(!parsed_mesh.is_error);

    /*const math::Vec3 forward = math::Vec3(0.0f, 0.0f, 1.0f);
    const math::Vec3 up = math::Vec3(0.0f, 1.0f, 0.0f);

    math::Transform<> transform;
    transform.set_basis(forward, forward.cross(up), up);
    transform = transform.transposed();*/

    MeshData mesh_data;
    for(const MaterialGroup& group : parsed_mesh.materials) {
        if(group.gltf_material_index < 0 || group.primitive_index < 0) {
            continue;
        }

        const tinygltf::Primitive& prim = gltf->meshes[parsed_mesh.gltf_index].primitives[group.primitive_index];

        try {
            auto vertices = import_vertices(*gltf, prim);
            if(vertices.is_empty()) {
                continue;
            }

            const bool recompute_tangents = vertices.size() && vertices[0].tangent.is_zero();

            Y_TODO(Make faster by using vertex streams)
            for(FullVertex& vertex : vertices) {
                vertex.position = base_change_transform.transform_point(vertex.position);
                vertex.normal = base_change_transform.transform_direction(vertex.normal);
                vertex.tangent.to<3>() = base_change_transform.transform_direction(vertex.tangent.to<3>());
                Y_TODO(Do we need to fix the binormal sign ?)
            }

            MeshData mesh(vertices, import_triangles(*gltf, prim));

            if(recompute_tangents) {
                mesh = compute_tangents(mesh);
            }

            mesh_data.add_sub_mesh(mesh.vertex_streams(), mesh.triangles());
        } catch(std::exception& e) {
            log_msg(fmt("Unable to build mesh: %" , e.what()), Log::Error);
        }
    }

    if(mesh_data.is_empty()) {
        log_msg("Mesh is empty, skipping" , Log::Warning);
        return core::Err();
    }

    return core::Ok(std::move(mesh_data));
}

core::Result<ImageData> ParsedScene::build_image_data(usize index, bool compress) const {
    y_profile();

    const Image& parsed_image = images[index];
    y_debug_assert(!parsed_image.is_error);

    ImageImportFlags flags = compress ? ImageImportFlags::Compress : ImageImportFlags::None;
    if(parsed_image.as_sRGB) {
        flags = flags | ImageImportFlags::ImportAsSRGB;
    }
    if(parsed_image.generate_mips) {
        flags = flags | ImageImportFlags::GenerateMipmaps;
    }

    const tinygltf::Image& image = gltf->images[parsed_image.gltf_index];
    if(!image.uri.empty()) {
        const FileSystemModel* fs = FileSystemModel::local_filesystem();
        const auto path = fs->parent_path(filename);
        const core::String image_path = path ? fs->join(path.unwrap(), image.uri) : core::String(image.uri);
        return import_image(image_path, flags);
    } else {
        const tinygltf::BufferView& view = gltf->bufferViews[image.bufferView];
        const tinygltf::Buffer& buffer = gltf->buffers[view.buffer];
        return import_image(core::Span<byte>(to_bytes(buffer.data.data()) + view.byteOffset, view.byteLength), flags);
    }
}

core::Result<MaterialData> ParsedScene::build_material_data(usize index) const {
    y_profile();

    const Material& parsed_material = materials[index];
    y_debug_assert(!parsed_material.is_error);

    const tinygltf::Material gltf_mat = gltf->materials[parsed_material.gltf_index];
    const tinygltf::PbrMetallicRoughness& pbr = gltf_mat.pbrMetallicRoughness;

    auto find_texture = [this](int tex_index) -> AssetPtr<Texture> {
        if(tex_index < 0) {
            return {};
        }

        const int image_index = gltf->textures[tex_index].source;
        const auto it = std::find_if(images.begin(), images.end(), [=](const auto& img) { return img.gltf_index == image_index; });
        if(it == images.end()) {
            return {};
        }

        y_debug_assert(it->gltf_index == image_index);
        return make_asset_with_id<Texture>(it->asset_id);
    };

    MaterialData data;
    data.set_texture(MaterialData::Diffuse, find_texture(pbr.baseColorTexture.index));
    data.set_texture(MaterialData::Normal, find_texture(gltf_mat.normalTexture.index));
    data.set_texture(MaterialData::Roughness, find_texture(pbr.metallicRoughnessTexture.index));
    data.set_texture(MaterialData::Metallic, find_texture(pbr.metallicRoughnessTexture.index));
    data.set_texture(MaterialData::Emissive, find_texture(gltf_mat.emissiveTexture.index));

    data.alpha_tested() = (gltf_mat.alphaMode != "OPAQUE");
    data.double_sided() = gltf_mat.doubleSided;

    data.metallic_mul() = float(pbr.metallicFactor);
    data.roughness_mul() = float(pbr.roughnessFactor);
    for(usize i = 0; i != 3; ++i) {
        data.emissive_mul()[i] = float(gltf_mat.emissiveFactor[i]);
    }

    return core::Ok(std::move(data));
}

core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}

}
}

