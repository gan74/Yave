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

#include "import.h"
#include "image_utils.h"

#include <yave/graphics/images/ImageData.h>
#include <yave/assets/AssetStore.h>
#include <yave/assets/AssetLoader.h>
#include <yave/ecs/EntityPrefab.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/RigidBodyComponent.h>
#include <yave/meshes/MeshData.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>
#include <yave/material/MaterialData.h>
#include <yave/utils/FileSystemModel.h>

#include <editor/components/EditorComponent.h>

#include <y/io2/File.h>
#include <y/io2/Buffer.h>
#include <y/serde3/archives.h>

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







core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}





// ----------------------------- ASSET STORE -----------------------------

template<typename T>
static AssetId import_asset(const core::String& name, const T& asset, AssetType type, const core::String& import_path, core::Span<AssetId> refs) {
    y_profile();
    y_profile_msg(fmt_c_str("Importing {} {}", asset_type_name(type), name));

    io2::Buffer buffer;
    {
        y_profile_zone("serialize");
        serde3::WritableArchive arc(buffer);
        if(const auto res = arc.serialize(asset); res.is_error()) {
            log_msg(fmt("Unable to serialize {}", name), Log::Error);
            return AssetId::invalid_id();
        }
    }

    {
        y_profile_zone("import");

        core::String suffix;
        for(usize i = 0;; ++i) {
            buffer.reset();
            if(const auto res = asset_store().import(buffer, asset_store().filesystem()->join(import_path, name + suffix), type, refs); res.is_ok()) {
                log_msg(fmt("Imported {} \"{}\" as {}", asset_type_name(type), name, stringify_id(res.unwrap())));
                return res.unwrap();
            } else if(res.error() == AssetStore::ErrorType::NameAlreadyExists) {
                suffix = fmt_to_owned(" ({})", i);
            } else {
                log_msg(fmt("Unable to import {}, error: {}", name, res.error()), Log::Error);
                break;
            }
        }
    }

    return AssetId::invalid_id();
}

static void deduplicate_refs(core::Vector<AssetId>& refs) {
    std::sort(refs.begin(), refs.end());
    refs.shrink_to(std::unique(refs.begin(), refs.end()) - refs.begin());
}

static core::Vector<AssetId> collect_refs(const MaterialData& data) {
    core::Vector<AssetId> refs;
    for(const auto& tex : data.textures()) {
        refs << tex.id();
    }
    deduplicate_refs(refs);
    return refs;
}

static void collect_refs(const ecs::EntityPrefab& prefab, core::Vector<AssetId>& refs) {
    for(const auto& child : prefab.asset_children()) {
        refs << child.id();
    }

    for(const auto& child : prefab.children()) {
        collect_refs(*child, refs);
    }

    for(const auto& box : prefab.components()) {
        if(const auto* mesh_box = dynamic_cast<const ecs::ComponentBox<StaticMeshComponent>*>(box.get())) {
            const StaticMeshComponent& mesh = mesh_box->component();
            refs << mesh.mesh().id();
            for(const auto& mat : mesh.materials()) {
                refs << mat.id();
            }
        }
    }
}

static core::Vector<AssetId> collect_refs(const ecs::EntityPrefab& prefab) {
    core::Vector<AssetId> refs;
    collect_refs(prefab, refs);
    deduplicate_refs(refs);
    return refs;
}




// ----------------------------- CONVERSIONS -----------------------------

static core::Result<ImageData> to_image_data(const gltf::Image& image, bool compress) {
    ImageImportFlags flags = ImageImportFlags::GenerateMipmaps;
    if(compress) {
        flags = flags | ImageImportFlags::Compress;
    }

    if(image.color_space == gltf::ColorSpace::sRGB) {
        flags = flags | ImageImportFlags::ImportAsSRGB;
    }

    if(image.is_normal_map) {
        flags = flags | ImageImportFlags::IsNormalMap;
    }

    if(!image.path.is_empty()) {
        return import_image(image.path, flags);
    }

    if(image.data.is_empty()) {
        log_msg(fmt("Image \"{}\" has no data", image.name), Log::Error);
        return core::Err();
    }

    return import_image(image.data, flags);
}

static MaterialData to_material_data(const gltf::Material& material, core::Span<AssetId> images) {
    auto find_texture = [&](const gltf::TextureRef& ref) -> AssetPtr<Texture> {
        if(!ref.is_valid()) {
            return {};
        }
        return make_asset_with_id<Texture>(images[ref.image]);
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

    if(material.is_specular_glossiness) {
        MaterialData::SpecularMaterialData data;
        fill_common(data.common);
        data.specular = find_texture(material.specular_glossiness);
        data.specular_color = find_texture(material.specular_glossiness);
        data.specular_color_factor = material.specular_factor;
        data.specular_factor = material.glossiness_factor;
        return MaterialData(std::move(data));
    }

    MaterialData::MetallicRoughnessMaterialData data;
    fill_common(data.common);
    data.metallic_roughness = find_texture(material.metallic_roughness);
    data.metallic_factor = material.metallic_factor;
    data.roughness_factor = material.roughness_factor;
    return MaterialData(std::move(data));
}

static MeshData to_mesh_data(const gltf::Mesh& mesh) {
    MeshData mesh_data;
    for(const gltf::Primitive& primitive : mesh.primitives) {
        mesh_data.add_sub_mesh(primitive.vertices, primitive.triangles);
    }
    return mesh_data;
}

template<typename T>
static T to_local_light(const gltf::Light& light) {
    T component;
    component.intensity() = light.intensity;
    component.color() = light.color;

    if(light.range > 0.0f) {
        component.range() = light.range;
    } else {
        const float intensity = light.color.dot(math::Vec3(light.intensity));
        component.range() = std::sqrt(intensity * 100.0f); // Put radius where lum < 1%
    }

    return component;
}

static SpotLightComponent to_spot_light(const gltf::Light& light) {
    SpotLightComponent component = to_local_light<SpotLightComponent>(light);
    component.half_angle() = light.outer_cone_angle;
    component.half_inner_angle() = std::min(light.inner_cone_angle, light.outer_cone_angle);
    return component;
}

static DirectionalLightComponent to_directional_light(const gltf::Light& light, const math::Transform<>& world_transform) {
    DirectionalLightComponent component;
    component.intensity() = light.intensity;
    component.color() = light.color;

    // glTF lights shine along -Z
    const math::Vec3 direction = -world_transform.forward();
    if(!direction.is_zero()) {
        component.direction() = direction.normalized();
    }

    return component;
}

// glTF spot lights shine along -Z while ours shine along +Z (forward)
static math::Transform<> to_spot_light_transform(const math::Transform<>& world_transform) {
    math::Transform<> tr = world_transform;
    tr.column(0) = -tr.column(0);
    tr.column(2) = -tr.column(2);
    return tr;
}

static void compute_world_transforms(const gltf::Scene& scene, core::MutableSpan<math::Transform<>> transforms, int index, const math::Transform<>& parent) {
    transforms[index] = parent * scene.nodes[index].local_transform;
    for(const int child : scene.nodes[index].children) {
        compute_world_transforms(scene, transforms, child, transforms[index]);
    }
}




// ----------------------------- SCENE IMPORT -----------------------------


struct ImportState : NonMovable {
    ImportState(gltf::Scene s, GltfImportSettings st) :
            scene(std::move(s)),
            settings(std::move(st)),
            name(clean_asset_name(scene.filename)),
            world_transforms(scene.nodes.size(), math::Transform<>()),
            images(scene.images.size(), AssetId::invalid_id()),
            materials(scene.materials.size(), AssetId::invalid_id()),
            meshes(scene.meshes.size(), AssetId::invalid_id()),
            prefabs(scene.nodes.size() + 1, AssetId::invalid_id()),
            status(std::make_shared<GltfImportStatus>()) {

        for(const int root : scene.roots) {
            compute_world_transforms(scene, world_transforms, root, math::Transform<>());
        }

        status->total = images.size() + materials.size() + meshes.size() + 1;
    }

    const gltf::Scene scene;
    const GltfImportSettings settings;
    const core::String name;

    core::Vector<math::Transform<>> world_transforms;

    core::Vector<AssetId> images;
    core::Vector<AssetId> materials;
    core::Vector<AssetId> meshes;
    core::Vector<AssetId> prefabs;

    const std::shared_ptr<GltfImportStatus> status;
};


template<typename F>
static void run_job(ImportState& state, F&& func) {
    const bool success = !state.status->cancelled && func() != AssetId::invalid_id();
    if(!success) {
        ++state.status->failed;
    }
    ++state.status->done;
}

static AssetId import_image(ImportState& state, usize index) {
    const gltf::Image& image = state.scene.images[index];
    if(const auto image_data = to_image_data(image, state.settings.compress_textures)) {
        state.images[index] = import_asset(image.name, image_data.unwrap(), AssetType::Image, state.settings.import_path, {});
    }
    return state.images[index];
}

static AssetId import_material(ImportState& state, usize index) {
    const gltf::Material& material = state.scene.materials[index];
    const MaterialData material_data = to_material_data(material, state.images);
    const core::Vector<AssetId> refs = collect_refs(material_data);
    return state.materials[index] = import_asset(material.name, material_data, AssetType::Material, state.settings.import_path, refs);
}

static AssetId import_mesh(ImportState& state, usize index) {
    const gltf::Mesh& mesh = state.scene.meshes[index];
    if(mesh.primitives.is_empty()) {
        log_msg(fmt("Mesh \"{}\" has no valid primitive", mesh.name), Log::Error);
        return AssetId::invalid_id();
    }
    return state.meshes[index] = import_asset(mesh.name, to_mesh_data(mesh), AssetType::Mesh, state.settings.import_path, {});
}

static int synthetic_root_index(const ImportState& state) {
    return int(state.scene.nodes.size());
}

static AssetId import_node(ImportState& state, int index);

static std::unique_ptr<ecs::EntityPrefab> create_prefab(ImportState& state, int index) {
    const gltf::Scene& scene = state.scene;

    auto prefab = std::make_unique<ecs::EntityPrefab>(ecs::EntityId::dummy(u32(index)));

    const bool is_synthetic = (index == synthetic_root_index(state));
    const core::Span<int> children = is_synthetic ? core::Span<int>(scene.roots) : core::Span<int>(scene.nodes[index].children);

    math::Transform<> transform = is_synthetic ? math::Transform<>() : state.world_transforms[index];

    if(!is_synthetic) {
        const gltf::Node& node = scene.nodes[index];

        if(node.mesh >= 0 && state.meshes[node.mesh] != AssetId::invalid_id()) {
            const gltf::Mesh& mesh = scene.meshes[node.mesh];

            core::Vector<AssetPtr<Material>> materials;
            for(const gltf::Primitive& primitive : mesh.primitives) {
                materials << (primitive.material < 0 ? AssetPtr<Material>() : make_asset_with_id<Material>(state.materials[primitive.material]));
            }

            prefab->add(StaticMeshComponent(make_asset_with_id<StaticMesh>(state.meshes[node.mesh]), std::move(materials)));

            if(state.settings.create_colliders) {
                prefab->add(RigidBodyComponent());
            }
        }

        if(node.light >= 0) {
            const gltf::Light& light = scene.lights[node.light];
            switch(light.type) {
                case gltf::LightType::Point:
                    prefab->add(to_local_light<PointLightComponent>(light));
                break;

                case gltf::LightType::Directional:
                    prefab->add(to_directional_light(light, state.world_transforms[index]));
                break;

                case gltf::LightType::Spot:
                    if(node.mesh >= 0) {
                        auto light_prefab = std::make_unique<ecs::EntityPrefab>(ecs::EntityId::dummy(u32(scene.nodes.size() + 1 + index)));
                        light_prefab->add(to_spot_light(light));
                        light_prefab->add(TransformableComponent(to_spot_light_transform(state.world_transforms[index])));
                        light_prefab->add(EditorComponent(light.name));
                        prefab->add_child(std::move(light_prefab));
                    } else {
                        prefab->add(to_spot_light(light));
                        transform = to_spot_light_transform(transform);
                    }
                break;
            }
        }
    }

    for(const int child_index : children) {
        if(state.settings.import_child_prefabs_as_assets) {
            if(const AssetId id = import_node(state, child_index); id != AssetId::invalid_id()) {
                prefab->add_child(make_asset_with_id<ecs::EntityPrefab>(id));
            }
        } else {
            prefab->add_child(create_prefab(state, child_index));
        }
    }

    prefab->add(TransformableComponent(transform));
    prefab->add(EditorComponent(is_synthetic ? state.name : scene.nodes[index].name));

    return prefab;
}

static AssetId import_node(ImportState& state, int index) {
    if(state.prefabs[index] != AssetId::invalid_id()) {
        return state.prefabs[index];
    }

    const std::unique_ptr<ecs::EntityPrefab> prefab = create_prefab(state, index);
    const core::String& name = (index == synthetic_root_index(state)) ? state.name : state.scene.nodes[index].name;
    return state.prefabs[index] = import_asset(name, *prefab, AssetType::Prefab, state.settings.import_path, collect_refs(*prefab));
}

static AssetId import_root(ImportState& state) {
    const int root = state.scene.roots.size() == 1 ? state.scene.roots.first() : synthetic_root_index(state);
    return state.status->root_prefab = import_node(state, root);
}

std::pair<concurrent::JobSystem::JobHandle, std::shared_ptr<GltfImportStatus>> schedule_gltf_import(concurrent::JobSystem& job_system, gltf::Scene scene, GltfImportSettings settings) {
    const auto state = std::make_shared<ImportState>(std::move(scene), std::move(settings));

    core::Vector<concurrent::JobSystem::JobHandle> image_jobs;
    core::Vector<concurrent::JobSystem::JobHandle> asset_jobs;

    for(usize i = 0; i != state->images.size(); ++i) {
        image_jobs.emplace_back(job_system.schedule([=] { run_job(*state, [&] { return import_image(*state, i); }); }));
    }

    for(usize i = 0; i != state->materials.size(); ++i) {
        asset_jobs.emplace_back(job_system.schedule([=] { run_job(*state, [&] { return import_material(*state, i); }); }, image_jobs));
    }

    for(usize i = 0; i != state->meshes.size(); ++i) {
        asset_jobs.emplace_back(job_system.schedule([=] { run_job(*state, [&] { return import_mesh(*state, i); }); }));
    }

    auto root_job = job_system.schedule([=] { run_job(*state, [&] { return import_root(*state); }); }, asset_jobs);
    return {std::move(root_job), state->status};
}

}
}

