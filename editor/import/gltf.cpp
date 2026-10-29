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

#include "gltf.h"

#include <yave/utils/FileSystemModel.h>

#include <y/io2/File.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>


// -------------- CGLTF --------------
#ifdef Y_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#ifdef Y_MSVC
#pragma warning(push)
#pragma warning(disable:4244) // conversion, possible loss of data
#pragma warning(disable:4267) // conversion from 'size_t' to 'int', possible loss of data
#pragma warning(disable:4996) // unsafe CRT functions
#endif

#define CGLTF_IMPLEMENTATION
#include <external/cgltf/cgltf.h>

#ifdef Y_MSVC
#pragma warning(pop)
#endif

#ifdef Y_GNU
#pragma GCC diagnostic pop
#endif


namespace editor {
namespace import {
namespace gltf {

using DataPtr = std::unique_ptr<cgltf_data, decltype(&cgltf_free)>;

static const char* result_str(cgltf_result result) {
    switch(result) {
        case cgltf_result_success:          return "success";
        case cgltf_result_data_too_short:   return "data too short";
        case cgltf_result_unknown_format:   return "unknown format";
        case cgltf_result_invalid_json:     return "invalid json";
        case cgltf_result_invalid_gltf:     return "invalid glTF";
        case cgltf_result_invalid_options:  return "invalid options";
        case cgltf_result_file_not_found:   return "file not found";
        case cgltf_result_io_error:         return "IO error";
        case cgltf_result_out_of_memory:    return "out of memory";
        case cgltf_result_legacy_gltf:      return "legacy glTF (1.0) is not supported";
        default:                            return "unknown error";
    }
}

static cgltf_result read_file(const cgltf_memory_options*, const cgltf_file_options*, const char* path, cgltf_size* size, void** data) {
    auto file = io2::File::open(path);
    if(!file) {
        return cgltf_result_file_not_found;
    }

    core::Vector<u8> content;
    if(!file.unwrap().read_all(content)) {
        return cgltf_result_io_error;
    }

    void* buffer = std::malloc(std::max(content.size(), 1_uu));
    if(!buffer) {
        return cgltf_result_out_of_memory;
    }

    std::memcpy(buffer, content.data(), content.size());

    *size = content.size();
    *data = buffer;
    return cgltf_result_success;
}

static void release_file(const cgltf_memory_options*, const cgltf_file_options*, void* data) {
    std::free(data);
}

template<typename T>
static int index_of(const T* first, const T* elem) {
    return elem ? int(elem - first) : -1;
}

static core::String name_or_default(const char* name, std::string_view prefix, usize index) {
    if(name && *name) {
        return name;
    }
    return fmt_to_owned("{}_{}", prefix, index);
}

static const char* supported_extensions[] = {
    "KHR_lights_punctual",
    "KHR_materials_emissive_strength",
    "KHR_materials_pbrSpecularGlossiness",
    "KHR_materials_specular",
    "KHR_materials_unlit",
    "KHR_mesh_quantization",
    "KHR_texture_transform",
};

static void check_extensions(const cgltf_data* data) {
    auto is_supported = [](const char* ext) {
        return std::any_of(std::begin(supported_extensions), std::end(supported_extensions), [=](const char* s) { return std::strcmp(s, ext) == 0; });
    };

    for(usize i = 0; i != data->extensions_required_count; ++i) {
        if(!is_supported(data->extensions_required[i])) {
            log_msg(fmt("Required glTF extension \"{}\" is not supported, scene might not import correctly", data->extensions_required[i]), Log::Warning);
        }
    }
}




// ----------------------------- GEOMETRY -----------------------------

static bool unpack_floats(const cgltf_accessor* accessor, usize components, core::Vector<float>& out) {
    if(cgltf_num_components(accessor->type) != components) {
        return false;
    }

    const usize float_count = accessor->count * components;
    out.make_empty();
    out.set_min_size(float_count, 0.0f);
    return cgltf_accessor_unpack_floats(accessor, out.data(), float_count) == float_count;
}

static bool unpack_indices(const cgltf_accessor* accessor, core::Vector<u32>& out) {
    out.make_empty();
    out.set_min_size(accessor->count, 0u);

    if(accessor->is_sparse) {
        // cgltf can't unpack sparse indices directly, floats are exact for any sane index range
        core::Vector<float> floats;
        if(!unpack_floats(accessor, 1, floats)) {
            return false;
        }
        std::transform(floats.begin(), floats.end(), out.begin(), [](float f) { return u32(f); });
        return true;
    }

    return cgltf_accessor_unpack_indices(accessor, out.data(), sizeof(u32), accessor->count) == accessor->count;
}

static core::Vector<IndexedTriangle> triangulate(cgltf_primitive_type type, core::Span<u32> indices) {
    core::Vector<IndexedTriangle> triangles;

    switch(type) {
        case cgltf_primitive_type_triangles:
            if(indices.size() % 3) {
                log_msg("Index count is not a multiple of 3, extra indices will be ignored", Log::Warning);
            }
            for(usize i = 0; i + 2 < indices.size(); i += 3) {
                triangles.push_back({indices[i], indices[i + 1], indices[i + 2]});
            }
        break;

        case cgltf_primitive_type_triangle_strip:
            for(usize i = 0; i + 2 < indices.size(); ++i) {
                // Every other triangle is flipped to keep a consistent winding
                const usize odd = i % 2;
                triangles.push_back({indices[i], indices[i + 1 + odd], indices[i + 2 - odd]});
            }
        break;

        case cgltf_primitive_type_triangle_fan:
            for(usize i = 0; i + 2 < indices.size(); ++i) {
                triangles.push_back({indices[i + 1], indices[i + 2], indices[0]});
            }
        break;

        default:
            y_fatal("Unsupported primitive type");
    }

    return triangles;
}

static math::Vec3 any_orthogonal(const math::Vec3& n) {
    const math::Vec3 axis = std::abs(n.x()) < 0.9f ? math::Vec3(1.0f, 0.0f, 0.0f) : math::Vec3(0.0f, 1.0f, 0.0f);
    return n.cross(axis).normalized();
}

static math::Vec3 safe_normalize(const math::Vec3& v, const math::Vec3& fallback) {
    const float len = v.length();
    return len > 1.0e-12f ? v / len : fallback;
}

static void compute_flat_normals(core::Vector<FullVertex>& vertices, core::Vector<IndexedTriangle>& triangles) {
    core::Vector<FullVertex> flat;
    flat.set_min_capacity(triangles.size() * 3);

    for(IndexedTriangle& tri : triangles) {
        const math::Vec3 a = vertices[tri[0]].position;
        const math::Vec3 b = vertices[tri[1]].position;
        const math::Vec3 c = vertices[tri[2]].position;
        const math::Vec3 normal = safe_normalize((b - a).cross(c - a), math::Vec3(0.0f, 0.0f, 1.0f));

        for(u32& index : tri) {
            FullVertex v = vertices[index];
            v.normal = normal;
            index = u32(flat.size());
            flat.push_back(v);
        }
    }

    vertices = std::move(flat);
}

static void compute_tangents(core::Vector<FullVertex>& vertices, core::Span<IndexedTriangle> triangles) {
    core::Vector<math::Vec3> tangents(vertices.size(), math::Vec3());
    core::Vector<math::Vec3> bitangents(vertices.size(), math::Vec3());

    for(const IndexedTriangle& tri : triangles) {
        const FullVertex& v0 = vertices[tri[0]];
        const FullVertex& v1 = vertices[tri[1]];
        const FullVertex& v2 = vertices[tri[2]];

        const math::Vec3 e1 = v1.position - v0.position;
        const math::Vec3 e2 = v2.position - v0.position;
        const math::Vec2 duv1 = v1.uv - v0.uv;
        const math::Vec2 duv2 = v2.uv - v0.uv;

        const float det = duv1.x() * duv2.y() - duv2.x() * duv1.y();
        if(std::abs(det) < 1.0e-12f) {
            continue;
        }

        const float r = 1.0f / det;
        const math::Vec3 t = (e1 * duv2.y() - e2 * duv1.y()) * r;
        const math::Vec3 b = (e2 * duv1.x() - e1 * duv2.x()) * r;

        for(const u32 index : tri) {
            tangents[index] += t;
            bitangents[index] += b;
        }
    }

    for(usize i = 0; i != vertices.size(); ++i) {
        const math::Vec3 n = vertices[i].normal;
        const math::Vec3 t = safe_normalize(tangents[i] - n * n.dot(tangents[i]), any_orthogonal(n));
        const float w = n.cross(t).dot(bitangents[i]) < 0.0f ? -1.0f : 1.0f;
        vertices[i].tangent = math::Vec4(t, w);
    }
}

static core::Result<Primitive> decode_primitive(const cgltf_data* data, const cgltf_primitive& prim) {
    const cgltf_accessor* positions = nullptr;
    const cgltf_accessor* normals = nullptr;
    const cgltf_accessor* tangents = nullptr;
    const cgltf_accessor* uvs = nullptr;

    for(usize i = 0; i != prim.attributes_count; ++i) {
        const cgltf_attribute& attrib = prim.attributes[i];
        switch(attrib.type) {
            case cgltf_attribute_type_position: positions = attrib.data; break;
            case cgltf_attribute_type_normal:   normals = attrib.data;   break;
            case cgltf_attribute_type_tangent:  tangents = attrib.data;  break;
            case cgltf_attribute_type_texcoord:
                if(attrib.index == 0) {
                    uvs = attrib.data;
                }
            break;

            default:
            break;
        }
    }

    if(!positions || !positions->count) {
        log_msg("Primitive has no positions", Log::Error);
        return core::Err();
    }

    const usize vertex_count = positions->count;
    for(const cgltf_accessor* acc : {normals, tangents, uvs}) {
        if(acc && acc->count != vertex_count) {
            log_msg("Primitive attributes have different vertex counts", Log::Error);
            return core::Err();
        }
    }

    Primitive primitive;
    primitive.material = index_of(data->materials, prim.material);
    primitive.vertices.set_min_size(vertex_count, FullVertex{});

    {
        core::Vector<float> floats;
        auto read = [&](const cgltf_accessor* acc, usize components, auto&& assign) {
            if(!acc) {
                return true;
            }
            if(!unpack_floats(acc, components, floats)) {
                return false;
            }
            for(usize i = 0; i != vertex_count; ++i) {
                assign(primitive.vertices[i], floats.data() + i * components);
            }
            return true;
        };

        const bool ok =
            read(positions, 3, [](FullVertex& v, const float* f) { v.position = math::Vec3(f[0], f[1], f[2]); }) &&
            read(normals,   3, [](FullVertex& v, const float* f) { v.normal = math::Vec3(f[0], f[1], f[2]); }) &&
            read(tangents,  4, [](FullVertex& v, const float* f) { v.tangent = math::Vec4(f[0], f[1], f[2], f[3]); }) &&
            read(uvs,       2, [](FullVertex& v, const float* f) { v.uv = math::Vec2(f[0], f[1]); });

        if(!ok) {
            log_msg("Unable to decode vertex attributes", Log::Error);
            return core::Err();
        }
    }

    {
        core::Vector<u32> indices;
        if(prim.indices) {
            if(!unpack_indices(prim.indices, indices)) {
                log_msg("Unable to decode indices", Log::Error);
                return core::Err();
            }
        } else {
            indices.set_min_size(vertex_count, 0u);
            for(usize i = 0; i != vertex_count; ++i) {
                indices[i] = u32(i);
            }
        }

        if(std::any_of(indices.begin(), indices.end(), [&](u32 i) { return i >= vertex_count; })) {
            log_msg("Index out of range", Log::Error);
            return core::Err();
        }

        primitive.triangles = triangulate(prim.type, indices);
    }

    if(primitive.triangles.is_empty()) {
        log_msg("Primitive has no triangles", Log::Error);
        return core::Err();
    }

    for(FullVertex& v : primitive.vertices) {
        v.normal = safe_normalize(v.normal, math::Vec3(0.0f, 0.0f, 1.0f));
    }

    if(!normals) {
        compute_flat_normals(primitive.vertices, primitive.triangles);
    }

    if(!tangents || !normals) {
        compute_tangents(primitive.vertices, primitive.triangles);
    }

    return core::Ok(std::move(primitive));
}

static Mesh decode_mesh(const cgltf_data* data, usize index) {
    const cgltf_mesh& gltf_mesh = data->meshes[index];

    Mesh mesh;
    mesh.name = name_or_default(gltf_mesh.name, "mesh", index);

    for(usize i = 0; i != gltf_mesh.primitives_count; ++i) {
        const cgltf_primitive& prim = gltf_mesh.primitives[i];

        if(prim.type != cgltf_primitive_type_triangles && prim.type != cgltf_primitive_type_triangle_strip && prim.type != cgltf_primitive_type_triangle_fan) {
            log_msg(fmt("Mesh \"{}\" primitive {} is not made of triangles and will be ignored", mesh.name, i), Log::Warning);
            continue;
        }

        if(prim.has_draco_mesh_compression) {
            log_msg(fmt("Mesh \"{}\" primitive {} uses Draco compression which is not supported", mesh.name, i), Log::Warning);
            continue;
        }

        if(auto primitive = decode_primitive(data, prim)) {
            mesh.primitives.push_back(std::move(primitive.unwrap()));
        } else {
            log_msg(fmt("Mesh \"{}\" primitive {} could not be imported", mesh.name, i), Log::Error);
        }
    }

    return mesh;
}




// ----------------------------- IMAGES -----------------------------

static core::Result<core::Vector<u8>> decode_data_uri(const char* uri) {
    const char* base64 = std::strstr(uri, ";base64,");
    if(!base64) {
        return core::Err();
    }
    base64 += std::strlen(";base64,");

    usize len = std::strlen(base64);
    while(len && base64[len - 1] == '=') {
        --len;
    }

    const usize size = (len * 3) / 4;
    void* decoded = nullptr;

    cgltf_options options = {};
    if(cgltf_load_buffer_base64(&options, size, base64, &decoded) != cgltf_result_success) {
        return core::Err();
    }
    y_defer(std::free(decoded));

    const u8* bytes = static_cast<const u8*>(decoded);
    return core::Ok(core::Vector<u8>(bytes, bytes + size));
}

static Image decode_image(const cgltf_data* data, usize index, const core::String& filename) {
    const cgltf_image& gltf_image = data->images[index];

    Image image;
    image.name = name_or_default(gltf_image.name, "texture", index);

    if(gltf_image.buffer_view) {
        const u8* bytes = cgltf_buffer_view_data(gltf_image.buffer_view);
        if(bytes) {
            image.data = core::Vector<u8>(bytes, bytes + gltf_image.buffer_view->size);
        }
    } else if(gltf_image.uri) {
        if(std::strncmp(gltf_image.uri, "data:", 5) == 0) {
            if(auto decoded = decode_data_uri(gltf_image.uri)) {
                image.data = std::move(decoded.unwrap());
            }
        } else {
            core::String uri = gltf_image.uri;
            uri.shrink(cgltf_decode_uri(uri.data()));

            const FileSystemModel* fs = FileSystemModel::local_filesystem();
            const auto path = fs->parent_path(filename);
            image.path = path ? fs->join(path.unwrap(), uri) : uri;
        }
    }

    if(image.path.is_empty() && image.data.is_empty()) {
        log_msg(fmt("Image \"{}\" has no data", image.name), Log::Warning);
    }

    return image;
}

class ImageResolver {
    enum Usage : u8 {
        Color,
        Data,
        NormalMap,

        UsageCount,
        Unused = UsageCount,
    };

    public:
        ImageResolver(const cgltf_data* data, Scene& scene) : _data(data), _scene(scene) {
            _variants.set_min_size(scene.images.size(), std::array<int, UsageCount>{-1, -1, -1});
            _base_usage.set_min_size(scene.images.size(), Unused);
        }

        TextureRef color(const cgltf_texture_view& view) {
            return resolve(view, Color);
        }

        TextureRef data(const cgltf_texture_view& view) {
            return resolve(view, Data);
        }

        TextureRef normal(const cgltf_texture_view& view) {
            return resolve(view, NormalMap);
        }

        void mark_unused_as_data() {
            for(usize i = 0; i != _base_usage.size(); ++i) {
                if(_base_usage[i] == Unused) {
                    apply_usage(_scene.images[i], Data);
                }
            }
        }

    private:
        TextureRef resolve(const cgltf_texture_view& view, Usage usage) {
            const cgltf_texture* texture = view.texture;
            if(!texture) {
                return {};
            }

            if(!texture->image) {
                log_msg(fmt("Texture \"{}\" only has an image in an unsupported format", texture->name ? texture->name : ""), Log::Warning);
                return {};
            }

            const int image_index = index_of(_data->images, texture->image);

            TextureRef ref;
            ref.uv_set = u32(view.has_transform && view.transform.has_texcoord ? view.transform.texcoord : view.texcoord);
            ref.image = variant(image_index, usage);

            if(ref.uv_set != 0) {
                log_msg(fmt("Texture \"{}\" uses UV set {}, only UV set 0 is supported", _scene.images[ref.image].name, ref.uv_set), Log::Warning);
            }

            return ref;
        }

        int variant(int image_index, Usage usage) {
            int& index = _variants[image_index][usage];
            if(index >= 0) {
                return index;
            }

            if(_base_usage[image_index] == Unused) {
                _base_usage[image_index] = usage;
                apply_usage(_scene.images[image_index], usage);
                index = image_index;
                return index;
            }

            static constexpr std::string_view suffixes[] = { "_color", "_linear", "_normal" };

            Image copy = _scene.images[image_index];
            copy.name = copy.name + suffixes[usage];
            apply_usage(copy, usage);

            index = int(_scene.images.size());
            _scene.images.push_back(std::move(copy));
            return index;
        }

        static void apply_usage(Image& image, Usage usage) {
            image.color_space = (usage == Color) ? ColorSpace::sRGB : ColorSpace::Linear;
            image.is_normal_map = (usage == NormalMap);
        }

        const cgltf_data* _data = nullptr;
        Scene& _scene;

        core::Vector<std::array<int, UsageCount>> _variants;
        core::Vector<Usage> _base_usage;
};




// ----------------------------- MATERIALS -----------------------------

static Material decode_material(const cgltf_data* data, usize index, ImageResolver& images) {
    const cgltf_material& gltf_mat = data->materials[index];

    Material mat;
    mat.name = name_or_default(gltf_mat.name, "material", index);

    if(gltf_mat.has_pbr_specular_glossiness) {
        const cgltf_pbr_specular_glossiness& pbr = gltf_mat.pbr_specular_glossiness;
        mat.is_specular_glossiness = true;
        mat.base_color = images.color(pbr.diffuse_texture);
        mat.base_color_factor = math::Vec4(pbr.diffuse_factor[0], pbr.diffuse_factor[1], pbr.diffuse_factor[2], pbr.diffuse_factor[3]);
        mat.specular_glossiness = images.color(pbr.specular_glossiness_texture);
        mat.specular_factor = math::Vec3(pbr.specular_factor[0], pbr.specular_factor[1], pbr.specular_factor[2]);
        mat.glossiness_factor = pbr.glossiness_factor;
    } else {
        const cgltf_pbr_metallic_roughness& pbr = gltf_mat.pbr_metallic_roughness;
        mat.base_color = images.color(pbr.base_color_texture);
        mat.metallic_roughness = images.data(pbr.metallic_roughness_texture);
        mat.base_color_factor = math::Vec4(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);
        mat.metallic_factor = pbr.metallic_factor;
        mat.roughness_factor = pbr.roughness_factor;
    }

    mat.normal = images.normal(gltf_mat.normal_texture);
    mat.normal_scale = gltf_mat.normal_texture.texture ? gltf_mat.normal_texture.scale : 1.0f;

    mat.occlusion = images.data(gltf_mat.occlusion_texture);
    mat.occlusion_strength = gltf_mat.occlusion_texture.texture ? gltf_mat.occlusion_texture.scale : 1.0f;

    mat.emissive = images.color(gltf_mat.emissive_texture);
    const float emissive_strength = gltf_mat.has_emissive_strength ? gltf_mat.emissive_strength.emissive_strength : 1.0f;
    mat.emissive_factor = math::Vec3(gltf_mat.emissive_factor[0], gltf_mat.emissive_factor[1], gltf_mat.emissive_factor[2]) * emissive_strength;

    switch(gltf_mat.alpha_mode) {
        case cgltf_alpha_mode_mask:  mat.alpha_mode = AlphaMode::Mask;   break;
        case cgltf_alpha_mode_blend: mat.alpha_mode = AlphaMode::Blend;  break;
        default:                     mat.alpha_mode = AlphaMode::Opaque; break;
    }

    mat.alpha_cutoff = gltf_mat.alpha_cutoff;
    mat.double_sided = gltf_mat.double_sided;
    mat.unlit = gltf_mat.unlit;

    return mat;
}




// ----------------------------- SCENE GRAPH -----------------------------

static Light decode_light(const cgltf_data* data, usize index) {
    const cgltf_light& gltf_light = data->lights[index];

    Light light;
    light.name = name_or_default(gltf_light.name, "light", index);
    light.color = math::Vec3(gltf_light.color[0], gltf_light.color[1], gltf_light.color[2]);
    light.intensity = gltf_light.intensity;
    light.range = gltf_light.range;
    light.inner_cone_angle = gltf_light.spot_inner_cone_angle;
    light.outer_cone_angle = gltf_light.spot_outer_cone_angle;

    switch(gltf_light.type) {
        case cgltf_light_type_directional:  light.type = LightType::Directional;    break;
        case cgltf_light_type_spot:         light.type = LightType::Spot;           break;
        default:                            light.type = LightType::Point;          break;
    }

    return light;
}

static math::Transform<> decode_transform(const cgltf_node& node) {
    if(node.has_matrix) {
        math::Matrix4<> mat;
        std::copy_n(node.matrix, 16, mat.begin());
        return mat;
    }

    const math::Vec3 translation = node.has_translation ? math::Vec3(node.translation[0], node.translation[1], node.translation[2]) : math::Vec3();
    const math::Vec4 rotation = node.has_rotation ? math::Vec4(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]) : math::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const math::Vec3 scale = node.has_scale ? math::Vec3(node.scale[0], node.scale[1], node.scale[2]) : math::Vec3(1.0f);

    return math::Transform<>(translation, rotation, scale);
}

static Node decode_node(const cgltf_data* data, usize index) {
    const cgltf_node& gltf_node = data->nodes[index];

    Node node;
    node.name = name_or_default(gltf_node.name, "node", index);
    node.local_transform = decode_transform(gltf_node);
    node.parent = index_of(data->nodes, gltf_node.parent);
    node.mesh = index_of(data->meshes, gltf_node.mesh);
    node.light = index_of(data->lights, gltf_node.light);

    for(usize i = 0; i != gltf_node.children_count; ++i) {
        node.children.push_back(index_of(data->nodes, gltf_node.children[i]));
    }

    if(!math::all_finite(node.local_transform)) {
        log_msg(fmt("Node \"{}\" has an invalid transform", node.name), Log::Warning);
        node.local_transform = {};
    }

    return node;
}

static core::Vector<int> find_roots(const cgltf_data* data) {
    const cgltf_scene* scene = data->scene ? data->scene : (data->scenes_count ? &data->scenes[0] : nullptr);

    core::Vector<int> roots;
    if(scene) {
        for(usize i = 0; i != scene->nodes_count; ++i) {
            roots.push_back(index_of(data->nodes, scene->nodes[i]));
        }
    } else {
        for(usize i = 0; i != data->nodes_count; ++i) {
            if(!data->nodes[i].parent) {
                roots.push_back(int(i));
            }
        }
    }

    return roots;
}




// ----------------------------- LOADING -----------------------------

static core::Result<DataPtr> parse_file(const core::String& filename) {
    y_profile();

    cgltf_options options = {};
    options.file.read = read_file;
    options.file.release = release_file;

    cgltf_data* raw_data = nullptr;
    if(const cgltf_result res = cgltf_parse_file(&options, filename.data(), &raw_data); res != cgltf_result_success) {
        log_msg(fmt("Unable to parse \"{}\": {}", filename, result_str(res)), Log::Error);
        return core::Err();
    }

    DataPtr data(raw_data, &cgltf_free);

    if(const cgltf_result res = cgltf_load_buffers(&options, data.get(), filename.data()); res != cgltf_result_success) {
        log_msg(fmt("Unable to load buffers for \"{}\": {}", filename, result_str(res)), Log::Error);
        return core::Err();
    }

    if(const cgltf_result res = cgltf_validate(data.get()); res != cgltf_result_success) {
        log_msg(fmt("\"{}\" is not a valid glTF file: {}", filename, result_str(res)), Log::Error);
        return core::Err();
    }

    return core::Ok(std::move(data));
}

core::Result<Scene> load_gltf(const core::String& filename) {
    y_profile();

    auto parsed = parse_file(filename);
    if(!parsed) {
        return core::Err();
    }

    const cgltf_data* data = parsed.unwrap().get();
    check_extensions(data);

    Scene scene;
    scene.filename = filename;

    for(usize i = 0; i != data->images_count; ++i) {
        scene.images.push_back(decode_image(data, i, filename));
    }

    {
        ImageResolver images(data, scene);
        for(usize i = 0; i != data->materials_count; ++i) {
            scene.materials.push_back(decode_material(data, i, images));
        }
        images.mark_unused_as_data();
    }

    for(usize i = 0; i != data->meshes_count; ++i) {
        scene.meshes.push_back(decode_mesh(data, i));
    }

    for(usize i = 0; i != data->lights_count; ++i) {
        scene.lights.push_back(decode_light(data, i));
    }

    for(usize i = 0; i != data->nodes_count; ++i) {
        scene.nodes.push_back(decode_node(data, i));
    }

    scene.roots = find_roots(data);

    return core::Ok(std::move(scene));
}

}
}
}

