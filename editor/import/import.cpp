/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include "transforms.h"

#include <yave/meshes/Vertex.h>
#include <yave/animations/Animation.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/SimpleMaterialData.h>
#include <yave/utils/FileSystemModel.h>

#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <y/serde3/archives.h>

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

// -------------- TINYGLTF --------------
#ifdef Y_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NOEXCEPTION
#include <external/tinygltf/tiny_gltf.h>

#ifdef Y_GNU
#pragma GCC diagnostic pop
#endif



#include <unordered_set>


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
    y_profile();

    int width, height, bpp;
    u8* data = stbi_load(filename.data(), &width, &height, &bpp, 4);
    y_defer(stbi_image_free(data));

    if(!data) {
        log_msg(fmt_c_str("Unable to load image \"%\".", filename), Log::Error);
        return core::Err();
    }

    const VkFormat format = ((flags & ImageImportFlags::ImportAsSRGB) == ImageImportFlags::ImportAsSRGB)
        ? VK_FORMAT_R8G8B8A8_SRGB
        : VK_FORMAT_R8G8B8A8_UNORM;

    ImageData img(math::Vec2ui(width, height), data, format);
    if((flags & ImageImportFlags::GenerateMipmaps) == ImageImportFlags::GenerateMipmaps) {
        img = compute_mipmaps(img);
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

    math::Vec4* vec4_elems = nullptr;
    math::Vec3* vec3_elems = nullptr;
    math::Vec2* vec2_elems = nullptr;
    if(name == "POSITION") {
        vec3_elems = &vertices[0].position;
    } else if(name == "NORMAL") {
        vec3_elems = &vertices[0].normal;
    } else if(name == "TANGENT") {
        vec4_elems = &vertices[0].tangent;
    } else if(name == "TEXCOORD_0") {
        vec2_elems = &vertices[0].uv;
    } else {
        log_msg(fmt("Attribute \"%\" is not supported", std::string_view(name)), Log::Warning);
        return;
    }

    if(vec4_elems) {
        decode_attrib_buffer_convert_internal(model, buffer, accessor, vec4_elems, vertices.size());
    }

    if(vec3_elems) {
        decode_attrib_buffer_convert_internal(model, buffer, accessor, vec3_elems, vertices.size());
    }

    if(vec2_elems) {
        decode_attrib_buffer_convert_internal(model, buffer, accessor, vec2_elems, vertices.size());
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



ParsedScene parse_scene(const core::String& filename) {
    y_profile();

    ParsedScene scene;
    scene.is_error = false;
    scene.gltf = decltype(scene.gltf)(new tinygltf::Model(), [](tinygltf::Model* ptr) { delete ptr; });

    tinygltf::Model& model = *scene.gltf;

    {
        tinygltf::TinyGLTF ctx;

        y_profile_zone("glTF import");
        const bool is_ascii = filename.ends_with(".gltf");
        const std::string file = filename.data();

        std::string err;
        std::string warn;
        const bool ok = is_ascii
                ? ctx.LoadASCIIFromFile(&model, &err, &warn, file)
                : ctx.LoadBinaryFromFile(&model, &err, &warn, file);

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

    for(usize i = 0; i != model.meshes.size(); ++i) {
        const tinygltf::Mesh& mesh = model.meshes[i];

        auto& parsed_mesh = scene.meshes.emplace_back();
        parsed_mesh.name = mesh.name.empty() ? fmt_to_owned("unnamed_mesh_%", i) : clean_asset_name(mesh.name);
        parsed_mesh.index = i;

        for(usize j = 0; j != mesh.primitives.size(); ++j) {
            const tinygltf::Primitive& prim = mesh.primitives[j];

            auto& parsed_sub = parsed_mesh.sub_meshes.emplace_back();
            parsed_sub.name = fmt("submesh_%", j);
            parsed_sub.index = j;
            parsed_sub.material_index = prim.material;

            if(prim.mode != TINYGLTF_MODE_TRIANGLES) {
                parsed_sub.is_error = true;
                continue;
            }
        }
    }

    {
        const FileSystemModel* fs = FileSystemModel::local_filesystem();
        auto path = fs->parent_path(filename);

        for(usize i = 0; i != model.images.size(); ++i) {
            const tinygltf::Image& image = model.images[i];

            core::String filename = fs->filename(image.uri);
            filename = filename.sub_str(0, filename.size() - fs->extention(filename).size());

            auto& parsed_image = scene.images.emplace_back();
            parsed_image.name = image.name.empty() ? (filename.is_empty() ? fmt_to_owned("unnamed_image_%", i) : filename) : clean_asset_name(image.name);
            parsed_image.index = i;

            parsed_image.path = path ? fs->join(path.unwrap(), image.uri) : core::String(image.uri);
            parsed_image.is_error |= !fs->exists(parsed_image.path).unwrap_or(false);
        }
    }

    for(usize i = 0; i != model.materials.size(); ++i) {
        const tinygltf::Material& material = model.materials[i];

        auto& parsed_material = scene.materials.emplace_back();
        parsed_material.name = material.name.empty() ? fmt_to_owned("unnamed_material_%", i) : clean_asset_name(material.name);
        parsed_material.index = i;

        if(material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            scene.images[material.pbrMetallicRoughness.baseColorTexture.index].as_sRGB = true;
        }
    }

    return scene;
}

core::Vector<core::Result<MeshData>> ParsedScene::build_mesh_data(usize index) const {
    y_profile();

    const Mesh& parsed_mesh = meshes[index];
    y_debug_assert(!parsed_mesh.is_error);

    /*const math::Vec3 forward = math::Vec3(0.0f, 0.0f, 1.0f);
    const math::Vec3 up = math::Vec3(0.0f, 1.0f, 0.0f);

    math::Transform<> transform;
    transform.set_basis(forward, forward.cross(up), up);
    transform = transform.transposed();*/

    core::Vector<core::Result<MeshData>> sub_meshes;
    for(const SubMesh& sub_mesh : parsed_mesh.sub_meshes) {
        if(sub_mesh.import) {
            y_debug_assert(!sub_mesh.is_error);
            const tinygltf::Primitive& prim = gltf->meshes[parsed_mesh.index].primitives[sub_mesh.index];
            try {
                auto vertices = import_vertices(*gltf, prim);
                const bool recompute_tangents = vertices.size() && vertices[0].tangent.is_zero();

                for(FullVertex& vertex : vertices) {
                    auto transform = [](math::Vec3 v) { return math::Vec3(-v.x(), v.z(), v.y()); };
                    vertex.position = transform(vertex.position);
                    vertex.normal = transform(vertex.normal);
                    vertex.tangent.to<3>() = transform(vertex.tangent.to<3>());
                    Y_TODO(Do we need to fix the binormal sign ?)
                }

                MeshData mesh(std::move(vertices), import_triangles(*gltf, prim));
                if(recompute_tangents) {
                    mesh = compute_tangents(mesh);
                }

                sub_meshes.emplace_back(core::Ok(std::move(mesh)));
            } catch(std::exception& e) {
                log_msg(fmt("Unable to build mesh: %" , e.what()), Log::Error);
                sub_meshes.emplace_back(core::Err());
            }
        }
    }

    return sub_meshes;
}

core::Result<ImageData> ParsedScene::build_image_data(usize index) const {
    y_profile();

    const Image& parsed_image = images[index];
    y_debug_assert(!parsed_image.is_error);

    ImageImportFlags flags = ImageImportFlags::None;
    if(parsed_image.as_sRGB) {
        flags = flags | ImageImportFlags::ImportAsSRGB;
    }
    if(parsed_image.generate_mips) {
        flags = flags | ImageImportFlags::GenerateMipmaps;
    }

    return import_image(parsed_image.path, flags);
}

core::Result<SimpleMaterialData> ParsedScene::build_material_data(usize index) const {
    y_profile();

    const Material& parsed_material = materials[index];
    y_debug_assert(!parsed_material.is_error);

    try {
        const tinygltf::Material gltf_mat = gltf->materials[parsed_material.index];
        const tinygltf::PbrMetallicRoughness& pbr = gltf_mat.pbrMetallicRoughness;

        auto find_texture = [this](int tex_index) -> AssetPtr<Texture> {
            if(tex_index < 0) {
                return {};
            }
            return make_asset_with_id<Texture>(images[tex_index].asset_id);
        };

        SimpleMaterialData data;
        data.set_texture(SimpleMaterialData::Diffuse, find_texture(pbr.baseColorTexture.index));
        data.set_texture(SimpleMaterialData::Normal, find_texture(gltf_mat.normalTexture.index));
        data.set_texture(SimpleMaterialData::Roughness, find_texture(pbr.metallicRoughnessTexture.index));
        data.set_texture(SimpleMaterialData::Metallic, find_texture(pbr.metallicRoughnessTexture.index));
        data.set_texture(SimpleMaterialData::Emissive, find_texture(gltf_mat.emissiveTexture.index));

        data.alpha_tested() = (gltf_mat.alphaMode == "MASK");
        data.double_sided() = gltf_mat.doubleSided;

        data.constants().metallic_mul = float(pbr.metallicFactor);
        data.constants().roughness_mul = float(pbr.roughnessFactor);
        for(usize i = 0; i != 3; ++i) {
            data.constants().emissive_mul[i] = float(gltf_mat.emissiveFactor[0]);
        }

        return core::Ok(std::move(data));
    } catch(std::exception& e) {
        log_msg(fmt("Unable to build material: %" , e.what()), Log::Error);
        return core::Err();
    }
}

core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}

}
}

