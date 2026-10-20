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
#include <yave/utils/FileSystemModel.h>

#include <y/io2/File.h>
#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <map>

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

static BoneTransform parse_bone_transform(const tinygltf::Node& node) {
    if(node.matrix.size() == 16) {
        const auto [position, rotation, scale] = parse_node_transform_matrix(node).decompose();
        return BoneTransform{position, scale, rotation};
    }

    BoneTransform transform;
    for(usize k = 0; k != node.translation.size(); ++k) {
        transform.position[k] = float(node.translation[k]);
    }

    for(usize k = 0; k != node.scale.size(); ++k) {
        transform.scale[k] = float(node.scale[k]);
    }

    if(node.rotation.size() == 4) {
        transform.rotation = math::Quaternion<>(math::Vec4(float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]), float(node.rotation[3])));
    }

    return transform;
}

static math::Transform<> parse_node_transform(const tinygltf::Node& node) {
    if(node.matrix.size() == 16) {
        return parse_node_transform_matrix(node);
    }

    y_debug_assert(node.matrix.empty());

    const math::Transform<> tr = parse_bone_transform(node).to_transform();
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

template<typename T>
static float read_component(const u8* data, bool normalized) {
    const float v = float(*reinterpret_cast<const T*>(data));
    if constexpr(std::is_integral_v<T> && sizeof(T) < sizeof(u32)) {
        if(normalized) {
            return std::max(v / float((1 << (sizeof(T) * 8 - std::is_signed_v<T>)) - 1), -1.0f);
        }
    }
    return v;
}

using ComponentReader = float (*)(const u8*, bool);

static ComponentReader component_reader(int component_type) {
    switch(component_type) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:              return read_component<i8>;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:     return read_component<u8>;
        case TINYGLTF_COMPONENT_TYPE_SHORT:             return read_component<i16>;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:    return read_component<u16>;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:      return read_component<u32>;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:             return read_component<float>;
        default:
            return nullptr;
    }
}

static core::Result<core::Vector<float>> read_accessor(const tinygltf::Model& model, int accessor_index, int expected_type) {
    if(accessor_index < 0 || usize(accessor_index) >= model.accessors.size()) {
        return core::Err();
    }

    const tinygltf::Accessor& accessor = model.accessors[accessor_index];
    const ComponentReader read = component_reader(accessor.componentType);
    if(accessor.type != expected_type || !read || accessor.sparse.isSparse) {
        log_msg("Unsupported accessor type", Log::Error);
        return core::Err();
    }

    const usize components = usize(tinygltf::GetNumComponentsInType(accessor.type));
    const usize elem_size = usize(tinygltf::GetComponentSizeInBytes(accessor.componentType));

    core::Vector<float> values;
    values.set_min_size(accessor.count * components);

    if(accessor.bufferView < 0) {
        return core::Ok(std::move(values));
    }

    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];
    const int stride = accessor.ByteStride(view);
    const usize offset = view.byteOffset + accessor.byteOffset;

    if(stride <= 0 || (accessor.count && offset + usize(stride) * (accessor.count - 1) + components * elem_size > buffer.data.size())) {
        log_msg("Accessor is out of bounds", Log::Error);
        return core::Err();
    }

    const u8* begin = buffer.data.data() + offset;
    for(usize i = 0; i != accessor.count; ++i) {
        for(usize c = 0; c != components; ++c) {
            values[i * components + c] = read(begin + i * usize(stride) + c * elem_size, accessor.normalized);
        }
    }

    return core::Ok(std::move(values));
}

template<typename F>
static bool import_stream(const tinygltf::Model& model, int accessor_index, int type, F&& store) {
    const auto values = read_accessor(model, accessor_index, type);
    if(values.is_error()) {
        return false;
    }

    const usize components = usize(tinygltf::GetNumComponentsInType(type));
    for(usize i = 0; i * components != values.unwrap().size(); ++i) {
        store(i, values.unwrap().data() + i * components);
    }
    return true;
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
    const auto positions = streams.stream<VertexStreamType::Position>();
    const auto normal_tangents = streams.stream<VertexStreamType::NormalTangent>();
    const auto uvs = streams.stream<VertexStreamType::Uv>();

    for(const auto& [name, id] : primitive.attributes) {
        bool ok = true;
        if(name == "POSITION") {
            ok = import_stream(model, id, TINYGLTF_TYPE_VEC3, [&](usize i, const float* v) {
                positions[i] = math::Vec3(v[0], v[1], v[2]);
            });
        } else if(name == "NORMAL") {
            ok = import_stream(model, id, TINYGLTF_TYPE_VEC3, [&](usize i, const float* v) {
                normal_tangents[i][0] = pack_2_10_10_10(math::Vec3(v[0], v[1], v[2]).normalized());
            });
        } else if(name == "TANGENT") {
            has_tangents = true;
            ok = import_stream(model, id, TINYGLTF_TYPE_VEC4, [&](usize i, const float* v) {
                normal_tangents[i][1] = pack_2_10_10_10(math::Vec3(v[0], v[1], v[2]).normalized(), v[3] < 0.0f);
            });
        } else if(name == "TEXCOORD_0") {
            ok = import_stream(model, id, TINYGLTF_TYPE_VEC2, [&](usize i, const float* v) {
                uvs[i] = math::Vec2(v[0], v[1]);
            });
        } else if(!name.starts_with("JOINTS_") && !name.starts_with("WEIGHTS_")) {
            log_msg(fmt("Attribute \"{}\" is not supported", name), Log::Warning);
        }

        if(!ok) {
            log_msg(fmt("Unable to import attribute \"{}\"", name), Log::Error);
            return core::Err();
        }
    }

    if(!has_tangents) {
        log_msg("Mesh doesn't have tangents", Log::Warning);
    }

    return core::Ok(std::move(streams));
}

static core::Result<core::Vector<IndexedTriangle>> import_triangles(const tinygltf::Model& model, const tinygltf::Primitive& primitive, usize vertex_count) {
    if(primitive.indices < 0) {
        // Non-indexed geometry: every 3 consecutive vertices form a triangle
        if(vertex_count < 3) {
            return core::Err();
        }

        core::Vector<IndexedTriangle> triangles;
        triangles.set_min_size(vertex_count / 3);
        for(usize i = 0; i != triangles.size(); ++i) {
            triangles[i] = {u32(i * 3), u32(i * 3 + 1), u32(i * 3 + 2)};
        }
        return core::Ok(std::move(triangles));
    }

    const tinygltf::Accessor accessor = model.accessors[primitive.indices];

    if(accessor.count < 3 || accessor.sparse.isSparse) {
        return core::Err();
    }

    if(accessor.count % 3) {
        log_msg("Index count is not a multiple of 3, extra indices will be ignored", Log::Warning);
    }

    core::Vector<IndexedTriangle> triangles;
    triangles.set_min_size(accessor.count / 3);

    const tinygltf::BufferView& buffer = model.bufferViews[accessor.bufferView];
    const int stride = accessor.ByteStride(buffer);
    if(stride <= 0) {
        return core::Err();
    }

    auto decode_indices = [&](auto convert_index) {
        const u8* begin = model.buffers[buffer.buffer].data.data() + buffer.byteOffset + accessor.byteOffset;
        for(usize i = 0; i != triangles.size() * 3; ++i) {
            triangles[i / 3][i % 3] = convert_index(begin + (i * usize(stride)));
        }
    };

    switch(accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_BYTE:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            decode_indices([](const u8* data) -> u32 { return *data; });
        break;

        case TINYGLTF_PARAMETER_TYPE_SHORT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            decode_indices([](const u8* data) -> u32 { return *reinterpret_cast<const u16*>(data); });
        break;

        case TINYGLTF_PARAMETER_TYPE_INT:
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            decode_indices([](const u8* data) -> u32 { return *reinterpret_cast<const u32*>(data); });
        break;

        default:
            return core::Err();
    }

    return core::Ok(std::move(triangles));
}



struct ImportedSkeleton {
    core::Vector<Bone> bones;
    core::Vector<u32> joint_to_bone;
};

static void add_skeleton_bones(const ParsedScene& scene, int node_index, u32 parent, core::Span<u8> in_skeleton, core::MutableSpan<u32> node_to_bone, core::Vector<Bone>& bones) {
    const u32 bone_index = u32(bones.size());
    node_to_bone[node_index] = bone_index;
    bones << Bone {
        scene.nodes[node_index].name,
        parent,
        parse_bone_transform(scene.gltf->nodes[node_index]),
    };

    for(const int child : scene.gltf->nodes[node_index].children) {
        if(in_skeleton[child]) {
            add_skeleton_bones(scene, child, bone_index, in_skeleton, node_to_bone, bones);
        }
    }
}

static core::Result<ImportedSkeleton> import_skeleton(const ParsedScene& scene, int skin_index, int mesh_node_index) {
    const tinygltf::Model& model = *scene.gltf;
    const tinygltf::Skin& skin = model.skins[skin_index];
    const usize node_count = model.nodes.size();

    if(skin.joints.empty()) {
        log_msg(fmt("Skin \"{}\" has no joints", skin.name), Log::Error);
        return core::Err();
    }

    auto parent_of = [&](int index) {
        const int parent = scene.nodes[index].parent_index;
        return (parent >= 0 && usize(parent) < node_count) ? parent : -1;
    };

    core::Vector<u8> in_skeleton;
    in_skeleton.set_min_size(node_count);
    for(const int joint : skin.joints) {
        if(joint < 0 || usize(joint) >= node_count) {
            log_msg(fmt("Skin \"{}\" has an invalid joint", skin.name), Log::Error);
            return core::Err();
        }
        for(int node = joint; node >= 0 && !in_skeleton[node]; node = parent_of(node)) {
            in_skeleton[node] = 1;
        }
    }

    ImportedSkeleton skeleton;

    {
        const math::Transform<> mesh_to_world = scene.nodes[mesh_node_index].transform;
        const auto [position, rotation, scale] = math::Transform<>(mesh_to_world.inverse()).decompose();
        skeleton.bones << Bone {
            "__mesh_root",
            u32(-1),
            BoneTransform{position, scale, rotation},
        };
    }

    core::Vector<u32> node_to_bone;
    node_to_bone.set_min_size(node_count, u32(-1));
    for(usize i = 0; i != node_count; ++i) {
        if(in_skeleton[i] && parent_of(int(i)) < 0) {
            add_skeleton_bones(scene, int(i), 0, in_skeleton, node_to_bone, skeleton.bones);
        }
    }

    if(skeleton.bones.size() > Skeleton::max_bones) {
        log_msg(fmt("Skin \"{}\" has too many bones ({}, max is {})", skin.name, skeleton.bones.size(), Skeleton::max_bones), Log::Error);
        return core::Err();
    }

    for(const int joint : skin.joints) {
        y_debug_assert(node_to_bone[joint] != u32(-1));
        skeleton.joint_to_bone << node_to_bone[joint];
    }

    {
        core::Vector<core::String> names;
        for(const Bone& bone : skeleton.bones) {
            names << bone.name;
        }
        std::sort(names.begin(), names.end());
        if(std::adjacent_find(names.begin(), names.end()) != names.end()) {
            log_msg(fmt("Skin \"{}\" has several bones with the same name, animations might not work", skin.name), Log::Warning);
        }
    }

    if(skin.inverseBindMatrices >= 0) {
        const auto inverse_binds = read_accessor(model, skin.inverseBindMatrices, TINYGLTF_TYPE_MAT4);
        if(inverse_binds.is_error() || inverse_binds.unwrap().size() != skin.joints.size() * 16) {
            log_msg(fmt("Skin \"{}\" has invalid inverse bind matrices", skin.name), Log::Error);
            return core::Err();
        }

        core::Vector<math::Transform<>> absolute;
        for(const Bone& bone : skeleton.bones) {
            absolute << (bone.has_parent() ? absolute[bone.parent] * bone.transform() : bone.transform());
        }

        bool matches = true;
        for(usize j = 0; j != skin.joints.size() && matches; ++j) {
            const math::Transform<> expected = absolute[skeleton.joint_to_bone[j]].inverse();
            const float* inverse_bind = inverse_binds.unwrap().data() + j * 16;
            matches = std::equal(expected.begin(), expected.end(), inverse_bind, [](float a, float b) {
                return std::abs(a - b) <= 1.0e-3f * std::max(1.0f, std::abs(b));
            });
        }

        if(!matches) {
            log_msg(fmt("Skin \"{}\" bind pose doesn't match the rest pose, skinning will be wrong", skin.name), Log::Warning);
        }
    }

    return core::Ok(std::move(skeleton));
}

static core::Result<void> import_skin_weights(const tinygltf::Model& model, const tinygltf::Primitive& primitive, usize vertex_count, core::Span<u32> joint_to_bone, core::Vector<SkinWeights>& skin) {
    struct Influences {
        std::array<float, SkinWeights::size> weights = {};
        std::array<u32, SkinWeights::size> bones = {};

        void add(u32 bone, float weight) {
            usize i = SkinWeights::size;
            while(i && weights[i - 1] < weight) {
                --i;
            }
            if(i == SkinWeights::size) {
                return;
            }
            for(usize k = SkinWeights::size - 1; k > i; --k) {
                weights[k] = weights[k - 1];
                bones[k] = bones[k - 1];
            }
            weights[i] = weight;
            bones[i] = bone;
        }
    };

    core::Vector<Influences> influences;
    influences.set_min_size(vertex_count);

    for(usize set = 0;; ++set) {
        const auto joints_it = primitive.attributes.find(fmt_to_owned("JOINTS_{}", set).data());
        const auto weights_it = primitive.attributes.find(fmt_to_owned("WEIGHTS_{}", set).data());
        if(joints_it == primitive.attributes.end() || weights_it == primitive.attributes.end()) {
            break;
        }

        const auto joints = read_accessor(model, joints_it->second, TINYGLTF_TYPE_VEC4);
        const auto weights = read_accessor(model, weights_it->second, TINYGLTF_TYPE_VEC4);
        if(joints.is_error() || weights.is_error() || joints.unwrap().size() != vertex_count * 4 || weights.unwrap().size() != vertex_count * 4) {
            log_msg("Invalid skin attributes", Log::Error);
            return core::Err();
        }

        for(usize v = 0; v != vertex_count; ++v) {
            for(usize k = 0; k != 4; ++k) {
                const float weight = weights.unwrap()[v * 4 + k];
                if(weight <= 0.0f) {
                    continue;
                }

                const usize joint = usize(joints.unwrap()[v * 4 + k]);
                if(joint >= joint_to_bone.size()) {
                    log_msg("Invalid skin joint index", Log::Error);
                    return core::Err();
                }

                influences[v].add(joint_to_bone[joint], weight);
            }
        }
    }

    skin.set_min_capacity(skin.size() + vertex_count);
    for(const Influences& inf : influences) {
        SkinWeights w = {};

        float total = 0.0f;
        for(const float weight : inf.weights) {
            total += weight;
        }

        if(total <= 0.0f) {
            w.weights[0] = u16(0xFFFF);
        } else {
            u32 sum = 0;
            for(usize k = 0; k != SkinWeights::size; ++k) {
                w.indices[k] = u16(inf.bones[k]);
                w.weights[k] = u16(std::round(inf.weights[k] / total * float(0xFFFF)));
                sum += w.weights[k];
            }
            w.weights[0] = u16(i32(w.weights[0]) + (i32(0xFFFF) - i32(sum)));
        }

        skin << w;
    }

    return core::Ok();
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
            texture.name = fmt_to_owned("texture_{}", scene.images.size());
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
                    if(const int image_index = compute_image_index(value.GetNumberAsInt()); image_index >= 0) {
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

    for(usize i = 0; i != scene.gltf->nodes.size(); ++i) {
        const tinygltf::Node& gltf_node = scene.gltf->nodes[i];
        if(gltf_node.mesh < 0 || gltf_node.skin < 0) {
            continue;
        }

        auto& mesh = scene.meshes[gltf_node.mesh];
        if(mesh.skin_index < 0) {
            mesh.skin_index = gltf_node.skin;
            mesh.skin_node_index = int(i);
        } else if(mesh.skin_index != gltf_node.skin) {
            log_msg(fmt("Mesh \"{}\" is used with several skins, only the first one will be imported", mesh.name), Log::Warning);
        }
    }

    for(const tinygltf::Animation& gltf_animation : scene.gltf->animations) {
        auto& animation = scene.animations.emplace_back();

        animation.name = gltf_animation.name;

        if(animation.name.is_empty()) {
            animation.name = fmt_to_owned("animation_{}", scene.animations.size());
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

    core::Result<ImportedSkeleton> skeleton = core::Err();
    core::Vector<SkinWeights> skin;
    if(meshes[index].skin_index >= 0) {
        skeleton = import_skeleton(*this, meshes[index].skin_index, meshes[index].skin_node_index);
        if(skeleton.is_error()) {
            log_msg(fmt("Unable to import skeleton of \"{}\", mesh will be imported as static", meshes[index].name), Log::Error);
        }
    }

    MeshData mesh_data;
    for(usize i = 0; i != mesh.primitives.size(); ++i) {
        const tinygltf::Primitive& primitive = mesh.primitives[i];

        if(primitive.mode != TINYGLTF_MODE_TRIANGLES) {
            return core::Err();
        }

        auto vertex_streams = import_vertices(*gltf, primitive);
        if(vertex_streams.is_error()) {
            return core::Err();
        }

        auto triangles = import_triangles(*gltf, primitive, vertex_streams.unwrap().vertex_count());
        if(triangles.is_error()) {
            return core::Err();
        }

        if(skeleton.is_ok()) {
            if(import_skin_weights(*gltf, primitive, vertex_streams.unwrap().vertex_count(), skeleton.unwrap().joint_to_bone, skin).is_error()) {
                log_msg(fmt("Unable to import skin of \"{}\", mesh will be imported as static", meshes[index].name), Log::Error);
                skeleton = core::Err();
            }
        }

        mesh_data.add_sub_mesh(std::move(vertex_streams.unwrap()), std::move(triangles.unwrap()));
    }

    if(skeleton.is_ok()) {
        mesh_data = MeshData(std::move(mesh_data), std::move(skeleton.unwrap().bones), std::move(skin));
    }

    return core::Ok(std::move(mesh_data));
}

core::Result<MaterialData> ParsedScene::create_material(int index) const {
    if(index < 0) {
        return core::Err();
    }

    const tinygltf::Material& material = gltf->materials[index];
    const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;

    const std::string emissive_strength_ext_name = "KHR_materials_emissive_strength";

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

        float emissive_factor = 1.0f;
        if(const auto it = material.extensions.find(emissive_strength_ext_name); it != material.extensions.end()) {
            emissive_factor = float(it->second.Get("emissiveStrength").GetNumberAsDouble());
        }

        for(usize i = 0; i != 3; ++i) {
            data.color_factor[i] = float(material.pbrMetallicRoughness.baseColorFactor[i]);
            data.emissive_factor[i] = float(material.emissiveFactor[i]) * emissive_factor;
        }

        data.alpha_tested = (material.alphaMode == "MASK");
        data.blend_mode = (material.alphaMode == "BLEND" ? BlendMode::SrcAlpha : BlendMode::None);
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

core::Result<yave::Animation> ParsedScene::create_animation(int index) const {
    if(index < 0) {
        return core::Err();
    }

    const tinygltf::Model& model = *gltf;
    const tinygltf::Animation& animation = model.animations[index];

    enum Path : usize {
        Translation,
        Rotation,
        Scale,

        PathCount
    };

    struct Curve {
        core::Vector<float> times;
        core::Vector<math::Vec4> values;
        bool step = false;
    };

    std::map<int, std::array<Curve, PathCount>> node_curves;
    float duration = 0.0f;

    for(const tinygltf::AnimationChannel& channel : animation.channels) {
        if(channel.target_node < 0 || usize(channel.target_node) >= model.nodes.size() || channel.sampler < 0 || usize(channel.sampler) >= animation.samplers.size()) {
            continue;
        }

        Path path = PathCount;
        int value_type = TINYGLTF_TYPE_VEC3;
        if(channel.target_path == "translation") {
            path = Translation;
        } else if(channel.target_path == "rotation") {
            path = Rotation;
            value_type = TINYGLTF_TYPE_VEC4;
        } else if(channel.target_path == "scale") {
            path = Scale;
        } else {
            log_msg(fmt("Animation channel \"{}\" is not supported", channel.target_path), Log::Warning);
            continue;
        }

        const tinygltf::AnimationSampler& sampler = animation.samplers[channel.sampler];
        const bool cubic = sampler.interpolation == "CUBICSPLINE";
        if(cubic) {
            log_msg("Cubic spline animation interpolation is not supported, using linear", Log::Warning);
        }

        auto times = read_accessor(model, sampler.input, TINYGLTF_TYPE_SCALAR);
        const auto values = read_accessor(model, sampler.output, value_type);
        const usize components = usize(tinygltf::GetNumComponentsInType(value_type));
        const usize values_per_key = cubic ? 3 : 1;

        if(times.is_error() || values.is_error() || times.unwrap().is_empty() || values.unwrap().size() != times.unwrap().size() * components * values_per_key) {
            log_msg(fmt("Invalid animation sampler in \"{}\"", animations[index].name), Log::Error);
            return core::Err();
        }

        Curve& curve = node_curves[channel.target_node][path];
        curve.step = sampler.interpolation == "STEP";
        curve.times = std::move(times.unwrap());
        for(usize k = 0; k != curve.times.size(); ++k) {
            const float* value = values.unwrap().data() + (k * values_per_key + (cubic ? 1 : 0)) * components;
            math::Vec4 v;
            for(usize c = 0; c != components; ++c) {
                v[c] = value[c];
            }
            curve.values << v;
        }

        duration = std::max(duration, curve.times.last());
    }

    if(node_curves.empty() || duration <= 0.0f) {
        log_msg(fmt("Animation \"{}\" is empty", animations[index].name), Log::Error);
        return core::Err();
    }

    struct Interpolant {
        math::Vec4 a;
        math::Vec4 b;
        float factor = 0.0f;

        math::Vec3 lerp() const {
            return (a * (1.0f - factor) + b * factor).to<3>();
        }
    };

    auto sample = [](const Curve& curve, float time) {
        const auto it = std::upper_bound(curve.times.begin(), curve.times.end(), time);
        if(it == curve.times.begin()) {
            return Interpolant{curve.values.first(), curve.values.first()};
        }
        if(it == curve.times.end()) {
            return Interpolant{curve.values.last(), curve.values.last()};
        }

        const usize next = usize(it - curve.times.begin());
        const usize prev = next - 1;
        if(curve.step) {
            return Interpolant{curve.values[prev], curve.values[prev]};
        }

        const float factor = (time - curve.times[prev]) / (curve.times[next] - curve.times[prev]);
        return Interpolant{curve.values[prev], curve.values[next], factor};
    };

    core::Vector<AnimationChannel> channels;
    for(const auto& [node_index, curves] : node_curves) {
        core::Vector<float> times;
        times << 0.0f;
        times << duration;
        for(const Curve& curve : curves) {
            for(const float t : curve.times) {
                times << std::clamp(t, 0.0f, duration);
                if(curve.step) {
                    times << std::clamp(std::nextafter(t, 0.0f), 0.0f, duration);
                }
            }
        }
        std::sort(times.begin(), times.end());
        times.shrink_to(std::unique(times.begin(), times.end()) - times.begin());

        const BoneTransform rest = parse_bone_transform(model.nodes[node_index]);

        core::Vector<AnimationChannel::BoneKey> keys;
        for(const float t : times) {
            BoneTransform transform = rest;

            if(!curves[Translation].times.is_empty()) {
                transform.position = sample(curves[Translation], t).lerp();
            }

            if(!curves[Rotation].times.is_empty()) {
                const Interpolant i = sample(curves[Rotation], t);
                transform.rotation = math::Quaternion<>(i.a).slerp(math::Quaternion<>(i.b), i.factor);
            }

            if(!curves[Scale].times.is_empty()) {
                transform.scale = sample(curves[Scale], t).lerp();
            }

            keys << AnimationChannel::BoneKey{t, transform};
        }

        channels << AnimationChannel(nodes[node_index].name, std::move(keys));
    }

    return core::Ok(yave::Animation(duration, std::move(channels)));
}

core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}

}
}

