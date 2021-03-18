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

#include "import.h"
#include "transforms.h"

#include <yave/meshes/Vertex.h>
#include <yave/animations/Animation.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/utils/FileSystemModel.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <y/serde3/archives.h>

// -------------- STB --------------
extern "C" {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <external/stb/stb_image.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

// -------------- TINYGLTF --------------
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NOEXCEPTION
#include <external/tinygltf/tiny_gltf.h>

#ifdef __GNUC__
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

Named<ImageData> import_image(const core::String& filename, ImageImportFlags flags) {
    y_profile();

    int width, height, bpp;
    u8* data = stbi_load(filename.data(), &width, &height, &bpp, 4);
    y_defer(stbi_image_free(data));

    if(!data) {
        y_throw_msg(fmt_c_str("Unable to load image \"%\".", filename));
    }

    const VkFormat format = ((flags & ImageImportFlags::ImportAsSRGB) == ImageImportFlags::ImportAsSRGB)
        ? VK_FORMAT_R8G8B8A8_SRGB
        : VK_FORMAT_R8G8B8A8_UNORM;
    ImageData img(math::Vec2ui(width, height), data, format);
    if((flags & ImageImportFlags::GenerateMipmaps) == ImageImportFlags::GenerateMipmaps) {
        img = compute_mipmaps(img);
    }
    return {clean_asset_name(filename), std::move(img)};
}

core::String supported_image_extensions() {
    return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}


template<typename T>
static void decode_attrib_buffer_convert_internal(const tinygltf::Model& model, const tinygltf::BufferView& buffer, T* vertex_elems, int type, bool normalize) {
    using value_type = typename T::value_type;
    const usize size = T::size();
    const usize components = type;

    const usize min_size = std::min(size, components);
    auto convert = [=](const u8* data) {
        T vec;
        for(usize i = 0; i != min_size; ++i) {
            vec[i] = reinterpret_cast<const value_type*>(data)[i];
        }
        if(normalize) {
            vec.normalize();
        }
        return vec;
    };

    {
        const usize elem_size = components * sizeof(value_type);
        u8* vertex_data = reinterpret_cast<u8*>(vertex_elems);
        const u8* data = model.buffers[buffer.buffer].data.data() + buffer.byteOffset;
        const usize stride = buffer.byteStride ? buffer.byteStride : elem_size;
        for(usize i = 0; i < buffer.byteLength; i += stride) {
            *reinterpret_cast<T*>(vertex_data) = convert(data + i);
            vertex_data += sizeof(Vertex);
        }
    }
}

static void decode_attrib_buffer(const tinygltf::Model& model, const std::string& name, const tinygltf::Accessor& accessor, Vertex* vertices) {
    const tinygltf::BufferView& buffer = model.bufferViews[accessor.bufferView];

    if(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
        y_throw_msg(fmt_c_str("Unsupported component type (%) for \"%\"", accessor.componentType, std::string_view(name)));
    }

    math::Vec3* vec3_elems = nullptr;
    math::Vec2* vec2_elems = nullptr;
    if(name == "POSITION") {
        vec3_elems = &vertices[0].position;
    } else if(name == "NORMAL") {
        vec3_elems = &vertices[0].normal;
    } else if(name == "TANGENT") {
        vec3_elems = &vertices[0].tangent;
    } else if(name == "TEXCOORD_0") {
        vec2_elems = &vertices[0].uv;
    } else {
        log_msg(fmt("Attribute \"%\" is not supported", std::string_view(name)), Log::Warning);
        return;
    }

    if(vec3_elems) {
        decode_attrib_buffer_convert_internal(model, buffer, vec3_elems, accessor.type, accessor.normalized);
    }

    if(vec2_elems) {
        decode_attrib_buffer_convert_internal(model, buffer, vec2_elems, accessor.type, accessor.normalized);
    }
}

static core::Vector<Vertex> import_vertices(const tinygltf::Model& model, const tinygltf::Primitive& prim) {
    core::Vector<Vertex> vertices;
    for(auto [name, id] : prim.attributes) {
        tinygltf::Accessor accessor = model.accessors[id];
        if(!accessor.count) {
            continue;
        }

        if(!vertices.size()) {
            std::fill_n(std::back_inserter(vertices), accessor.count, Vertex());
        } else if(vertices.size() != accessor.count) {
            y_throw_msg("Invalid attribute count");
        }

        if(accessor.normalized) {
            y_throw_msg("Normalization not supported");
        }

        decode_attrib_buffer(model, name, accessor, vertices.data());
    }
    return vertices;
}


template<typename F>
static void decode_index_buffer(const tinygltf::Model& model, const tinygltf::BufferView& buffer, IndexedTriangle* triangles, usize elem_size, F convert_index) {
    u32* indices = reinterpret_cast<u32*>(triangles);
    const u8* data = model.buffers[buffer.buffer].data.data() + buffer.byteOffset;
    const usize stride = buffer.byteStride ? buffer.byteStride : elem_size;
    for(usize i = 0; i < buffer.byteLength; i += stride) {
        *indices = convert_index(data + i);
        ++indices;
    }
}

static core::Vector<IndexedTriangle> import_triangles(const tinygltf::Model& model, const tinygltf::Primitive& prim) {
    tinygltf::Accessor accessor = model.accessors[prim.indices];
    if(!accessor.count) {
        y_throw_msg("Non indexed primitives are not supported");
    }

    core::Vector<IndexedTriangle> triangles;
    std::fill_n(std::back_inserter(triangles), accessor.count / 3, IndexedTriangle{});
    const tinygltf::BufferView& buffer = model.bufferViews[accessor.bufferView];
    switch(accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_BYTE:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            decode_index_buffer(model, buffer, triangles.data(), 1, [](const u8* data) -> u32 { return *data; });
        break;

        case TINYGLTF_PARAMETER_TYPE_SHORT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            decode_index_buffer(model, buffer, triangles.data(), 2, [](const u8* data) -> u32 { return *reinterpret_cast<const u16*>(data); });
        break;

        case TINYGLTF_PARAMETER_TYPE_INT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            decode_index_buffer(model, buffer, triangles.data(), 4, [](const u8* data) -> u32 { return *reinterpret_cast<const u32*>(data); });
        break;

        default:
            y_throw_msg("Index component type not supported");
    }

    return triangles;
}

SceneData import_scene(const core::String& filename, SceneImportFlags flags) {
    y_profile();

    tinygltf::Model model;

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
        if(!ok) {
            y_throw_msg(err);
        }
    }


    SceneData scene;

    if((flags & SceneImportFlags::ImportMeshes) == SceneImportFlags::ImportMeshes) {
        y_profile_zone("Mesh import");

        for(const tinygltf::Mesh& mesh : model.meshes) {
            for(usize i = 0; i != mesh.primitives.size(); ++i) {
                const tinygltf::Primitive& prim = mesh.primitives[i];

                if(prim.mode != TINYGLTF_MODE_TRIANGLES) {
                    log_msg("Primitive is not a triangle", Log::Warning);
                    continue;
                }

                const core::String name = mesh.name.empty()
                    ? fmt("unnamed_mesh_%", i)
                    : (i ? fmt("%_%", std::string_view(mesh.name), i) : std::string_view(mesh.name));

                auto vertices = import_vertices(model, prim);

                if((flags & SceneImportFlags::FlipUVs) == SceneImportFlags::FlipUVs) {
                    y_profile_zone("Flip UV");
                    for(Vertex& v : vertices) {
                        v.uv.y() = 1.0f - v.uv.y();
                    }
                }

                MeshData mesh(std::move(vertices), import_triangles(model, prim));
                if(vertices.size() && vertices[0].tangent.is_zero()) {
                    mesh = compute_tangents(mesh);
                }

                scene.meshes.emplace_back(clean_asset_name(name), std::move(mesh));
            }
        }
    }

    std::unordered_set<int> diffuse_images;
    const bool srgb_diffuse = (flags & SceneImportFlags::ImportDiffuseAsSRGB) == SceneImportFlags::ImportDiffuseAsSRGB;
    if(srgb_diffuse) {
        for(const tinygltf::Material& material : model.materials) {
            const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
            const int diffuse_index = pbr.baseColorTexture.index;
            if(srgb_diffuse && diffuse_index >= 0) {
                diffuse_images.insert(diffuse_index);
            }
        }
    }

    if((flags & SceneImportFlags::ImportImages) == SceneImportFlags::ImportImages) {
        y_profile_zone("Image import");

        const FileSystemModel* fs = FileSystemModel::local_filesystem();
        auto path = fs->parent_path(filename);

        for(usize i = 0; i != model.images.size(); ++i) {
            const tinygltf::Image& image = model.images[i];
            auto full_uri = path ? fs->join(path.unwrap(), image.uri) : core::String(image.uri);

            ImageImportFlags flags = ImageImportFlags::GenerateMipmaps;
            if(srgb_diffuse && diffuse_images.find(int(i)) != diffuse_images.end()) {
                flags = flags | ImageImportFlags::ImportAsSRGB;
            }

            scene.images.emplace_back(import_image(full_uri, flags));

            if(!image.name.empty()) {
                auto& last = scene.images.last();
                last  = Named<ImageData>(clean_asset_name(image.name), std::move(last.obj()));
            }
        }
    }

    if((flags & SceneImportFlags::ImportMaterials) == SceneImportFlags::ImportMaterials) {
        y_profile_zone("Material import");

        for(usize i = 0; i != model.materials.size(); ++i) {
            const tinygltf::Material& material = model.materials[i];
            const core::String name = material.name.empty() ? core::String(fmt("unnamed_material_%", i)) : clean_asset_name(material.name);

            auto tex_name = [&](int index) {
                if(index >= 0) {
                    const tinygltf::Texture& tex = model.textures[index];
                    if(tex.source >= 0 && usize(tex.source) < scene.images.size()) {
                        return scene.images[tex.source].name();
                    }
                }
                return core::String();
            };

            auto& last = scene.materials.emplace_back(name, MaterialData()).obj();
            {
                const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;

                last.textures[SimpleMaterialData::Normal] = tex_name(material.normalTexture.index);
                last.textures[SimpleMaterialData::Diffuse] = tex_name(pbr.baseColorTexture.index);
                last.textures[SimpleMaterialData::Metallic] = last.textures[SimpleMaterialData::Roughness] = tex_name(pbr.metallicRoughnessTexture.index);
                last.textures[SimpleMaterialData::Emissive] = tex_name(material.emissiveTexture.index);
                last.metallic = float(pbr.metallicFactor);
                last.roughness = float(pbr.roughnessFactor);

                for(usize i = 0; i != 3; ++i) {
                    last.emissive[i] = float(material.emissiveFactor[0]);
                }
            }
        }
    }

    if((flags & SceneImportFlags::ImportPrefabs) == SceneImportFlags::ImportPrefabs) {
        y_profile_zone("Prefab import");

        PrefabData& prefab = scene.prefabs.emplace_back(clean_asset_name(filename), PrefabData{});
        for(const tinygltf::Mesh& object : model.meshes) {
            for(const tinygltf::Primitive& prim : object.primitives) {
                if(prim.mode != TINYGLTF_MODE_TRIANGLES) {
                    continue;
                }

                if(usize(prim.material) < scene.materials.size()) {
                    const core::String& mesh_name = scene.meshes[prefab.sub_meshes.size()].name();
                    const core::String& mat_name = scene.materials[prim.material].name();

                    prefab.sub_meshes.emplace_back(SubMeshData{mesh_name, mat_name});
                }
            }
        }

        if(prefab.sub_meshes.is_empty()) {
            scene.prefabs.pop();
        }
    }


    return scene;
}

core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}

}
}

