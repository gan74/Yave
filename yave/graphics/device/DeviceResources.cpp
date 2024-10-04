/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "DeviceResources.h"
#include "LifetimeManager.h"
#include "DebugUtils.h"

#include <yave/graphics/shaders/SpirVData.h>
#include <yave/graphics/shaders/ShaderModule.h>
#include <yave/graphics/shaders/ShaderProgram.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/Material.h>
#include <yave/material/MaterialTemplate.h>
#include <yave/meshes/MeshData.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/graphics/images/IBLProbe.h>

#include <y/math/random.h>
#include <y/core/Chrono.h>
#include <y/io2/File.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <algorithm>

namespace yave {

using ComputePrograms = DeviceResources::ComputePrograms;
using RaytracingPrograms = DeviceResources::RaytracingPrograms;
using MaterialTemplates = DeviceResources::MaterialTemplates;
using Textures = DeviceResources::Textures;

struct DeviceMaterialData {
    const std::string_view frag;
    const std::string_view vert;

    const DepthTestMode depth_test;
    const BlendMode blend_mode;
    const CullMode cull_mode;
    const bool depth_write;
    const PrimitiveType primitive_type = PrimitiveType::Triangles;

    static constexpr DeviceMaterialData screen(std::string_view frag, BlendMode mode) {
        return DeviceMaterialData{frag, "screen", DepthTestMode::None, mode, CullMode::None, false};
    }

    static constexpr DeviceMaterialData screen(std::string_view frag, bool blended = false) {
        return DeviceMaterialData::screen(frag, blended ? BlendMode::Add : BlendMode::None);
    }
};


static constexpr std::array<DeviceMaterialData, usize(MaterialTemplates::MaxMaterialTemplates)> material_datas = {
    DeviceMaterialData{"opaque", "opaque", DepthTestMode::Standard, BlendMode::None, CullMode::Back, true},
    DeviceMaterialData{"opaque_ALPHA_TEST", "opaque_ALPHA_TEST", DepthTestMode::Standard, BlendMode::None, CullMode::Back, true},
    DeviceMaterialData{"opaque_ALPHA_TEST", "opaque_ALPHA_TEST", DepthTestMode::Standard, BlendMode::None, CullMode::None, true},

    DeviceMaterialData{"opaque_SPECULAR", "opaque_SPECULAR", DepthTestMode::Standard, BlendMode::None, CullMode::Back, true},
    DeviceMaterialData{"opaque_SPECULAR_ALPHA_TEST", "opaque_SPECULAR_ALPHA_TEST", DepthTestMode::Standard, BlendMode::None, CullMode::Back, true},
    DeviceMaterialData{"opaque_SPECULAR_ALPHA_TEST", "opaque_SPECULAR_ALPHA_TEST", DepthTestMode::Standard, BlendMode::None, CullMode::None, true},

    DeviceMaterialData{"deferred_light_POINT", "deferred_light_POINT", DepthTestMode::Reversed, BlendMode::Add, CullMode::Front, false},
    DeviceMaterialData{"deferred_light_SPOT", "deferred_light_SPOT", DepthTestMode::Reversed, BlendMode::Add, CullMode::Front, false},
    DeviceMaterialData::screen("deferred_ambient", true),
    DeviceMaterialData::screen("atmosphere"),
    DeviceMaterialData::screen("tonemap"),
    DeviceMaterialData::screen("passthrough"),
    DeviceMaterialData::screen("passthrough", true),
    DeviceMaterialData::screen("downsample"),
    DeviceMaterialData::screen("bloom_upscale", BlendMode::SrcAlpha),
    DeviceMaterialData::screen("bloom_downscale"),
    DeviceMaterialData::screen("blur_HORIZONTAL", true),
    DeviceMaterialData::screen("blur_VERTICAL", true),
    DeviceMaterialData{"wireframe", "wireframe", DepthTestMode::Standard, BlendMode::SrcAlpha, CullMode::Back, true, PrimitiveType::Lines},
    DeviceMaterialData::screen("temporal_desocclusion"),
    DeviceMaterialData::screen("temporal"),
    DeviceMaterialData::screen("taa_resolve"),
    DeviceMaterialData{"id", "id", DepthTestMode::Standard, BlendMode::None, CullMode::Back, true},
};

static constexpr std::array<std::string_view, usize(ComputePrograms::MaxComputePrograms)> compute_datas = {
    "ibl_convolution_EQUIREC",
    "ibl_convolution_CUBE",
    "brdf_integrator",
    "deferred_locals",
    "deferred_locals_DEBUG",
    "linearize_depth",
    "ssao",
    "ssao_upsample",
    "ssao_upsample_COMBINE_HIGH",
    "copy",
    "histogram",
    "exposure_params",
    "exposure_debug",
    "depth_bounds",
    "prev_camera",
    "update_transforms",
    "filter_rtao_HORIZONTAL",
    "filter_rtao_VERTICAL",
    "rtao",
};


static constexpr std::array<std::string_view, usize(RaytracingPrograms::MaxRaytracingPrograms)> raytracing_datas = {
    "basic_rt",
};

// ABGR
static constexpr std::array<math::Vec4ui, usize(Textures::MaxTextures)> texture_colors = {
    math::Vec4ui{0x00000000, 0x00000000, 0x00000000, 0x00000000},
    math::Vec4ui{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
    math::Vec4ui{0xFF7F7F7F, 0xFF7F7F7F, 0xFF7F7F7F, 0xFF7F7F7F},
    math::Vec4ui{0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF},
    math::Vec4ui{0x00000000, 0x00000000, 0x00000000, 0x00000000},
    math::Vec4ui{0x00FF7F7F, 0x00FF7F7F, 0x00FF7F7F, 0x00FF7F7F},
    math::Vec4ui{0xFFFFE5A6, 0xFFFFE5A6, 0xFF475163, 0xFF475163},
};

static constexpr std::array<ImageFormat, usize(Textures::MaxTextures)> texture_formats = {
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R32_UINT,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
};


// implemented in DeviceResourcesData.cpp
MeshData cube_mesh_data();
MeshData sphere_mesh_data();
MeshData simple_sphere_mesh_data();
MeshData cone_mesh_data();

static Texture create_brdf_lut(const ComputeProgram& brdf_integrator, usize size = 512) {
    y_profile();

    core::DebugTimer _("create_ibl_lut()");

    StorageTexture image(ImageFormat(VK_FORMAT_R16G16_UNORM), {size, size});

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    {
        const auto region = recorder.region("create_brdf_lut");
        recorder.dispatch_threads(brdf_integrator, image.size(), DescriptorSet(StorageView(image)));
    }
    recorder.submit().wait();

    return image;
}

static Texture create_white_noise(usize size = 256) {
    y_profile();

    core::DebugTimer _("create_white_noise()");

    auto data = std::make_unique_for_overwrite<u32[]>(size * size);

    math::FastRandom rng;
    std::generate_n(data.get(), size * size, rng);

    return Texture(ImageData(math::Vec2ui(size, size), reinterpret_cast<const u8*>(data.get()), VK_FORMAT_R8G8B8A8_UNORM));
}



DeviceResources::DeviceResources() {
    y_profile();

    _computes = std::make_unique<ComputeProgram[]>(compute_datas.size());
    _material_templates = std::make_unique<MaterialTemplate[]>(material_datas.size());
    _textures = std::make_unique<AssetPtr<Texture>[]>(texture_colors.size());

    if(raytracing_enabled()) {
        _raytracing_programs = std::make_unique<RaytracingProgram[]>(raytracing_datas.size());
    }

    const auto set_name = [debug = debug_utils()](auto handle, const char* name) {
        if(debug) {
            debug->set_resource_name(handle, name);
        }
    };

    {
        y_profile_zone("Images");
        for(usize i = 0; i != texture_colors.size(); ++i) {
            const u8* data = reinterpret_cast<const u8*>(texture_colors[i].data());
            _textures[i] = make_asset<Texture>(ImageData(math::Vec2ui(2), data, texture_formats[i]));

            set_name(_textures[i]->vk_image(), "Resource Image");
            set_name(_textures[i]->vk_view(), "Resource Image View");
        }
    }

    core::FlatHashMap<core::String, SpirVData> spirvs;
    auto load_spirv = [&](std::string_view name) -> const SpirVData& {
        y_debug_assert(!name.empty());
        const core::String filename = fmt_to_owned("{}.spv", name);
        auto& spirv = spirvs[filename];
        if(spirv.is_empty()) {
            spirv = SpirVData::deserialized(io2::File::open(filename).expected(fmt_c_str("Unable to open SPIR-V file ({})", filename)));
        }
        return spirv;
    };

    {
        for(usize i = 0; i != compute_datas.size(); ++i) {
            if(i < MaxNonRTComputePrograms || raytracing_enabled()) {
                _computes[i] = ComputeProgram(ComputeShader(load_spirv(compute_datas[i])));
                set_name(_computes[i].vk_pipeline(), fmt_c_str("{}", compute_datas[i]));
            }
        }
    }

    if(_raytracing_programs) {
        for(usize i = 0; i != raytracing_datas.size(); ++i) {
            const auto& spirv = load_spirv(raytracing_datas[i]);
            _raytracing_programs[i] = RaytracingProgram(
                RayGenShader(spirv), MissShader(spirv), ClosestHitShader(spirv)
            );
            set_name(_raytracing_programs[i].vk_pipeline(), fmt_c_str("{}", raytracing_datas[i]));
        }
    }

    {
        y_profile_zone("Material templates");
        for(usize i = 0; i != material_datas.size(); ++i) {
            const auto& data = material_datas[i];
            auto template_data = MaterialTemplateData()
                .set_frag_data(load_spirv(data.frag))
                .set_vert_data(load_spirv(data.vert))
                .set_depth_mode(data.depth_test)
                .set_cull_mode(data.cull_mode)
                .set_blend_mode(data.blend_mode)
                .set_depth_write(data.depth_write)
                .set_primitive_type(data.primitive_type);
            ;
            _material_templates[i] = MaterialTemplate(std::move(template_data));
            _material_templates[i].set_name(fmt_c_str("{} | {}", data.vert, data.frag));
        }
    }

    {
        y_profile_zone("Materials");
        _materials = std::make_unique<AssetPtr<Material>[]>(usize(MaxMaterials));
        _materials[0] = make_asset<Material>(&_material_templates[usize(TexturedMaterialTemplate)]);
    }

    {
        y_profile_zone("Meshes");
        _meshes = std::make_unique<AssetPtr<StaticMesh>[]>(usize(MaxMeshes));
        _meshes[0] = make_asset<StaticMesh>(cube_mesh_data());
        _meshes[1] = make_asset<StaticMesh>(sphere_mesh_data());
        _meshes[2] = make_asset<StaticMesh>(simple_sphere_mesh_data());
        _meshes[3] = make_asset<StaticMesh>(cone_mesh_data());
    }

    {
        y_profile_zone("Procedurals");
        _brdf_lut = create_brdf_lut(operator[](BRDFIntegratorProgram));
        set_name(_brdf_lut.vk_image(), "BRDF LUT");
        set_name(_brdf_lut.vk_view(), "BRDF LUT View");

        _white_noise = create_white_noise();
        set_name(_white_noise.vk_image(), "White Noise");
        set_name(_white_noise.vk_view(), "White Noise View");
    }

    {
        y_profile_zone("Probes");
        _probe = make_asset<IBLProbe>(IBLProbe::from_equirec(*operator[](SkyIBLTexture)));
        set_name(_probe->vk_image(), "Default Probe");
        set_name(_probe->vk_view(), "Default Probe");

        _empty_probe = make_asset<IBLProbe>(IBLProbe::from_equirec(*operator[](BlackTexture)));
        set_name(_empty_probe->vk_image(), "Empty Probe");
        set_name(_empty_probe->vk_view(), "Empty Probe");
    }
}


DeviceResources::~DeviceResources() {
    // _materials = {};
    // lifetime_manager().wait_cmd_buffers();
}

TextureView DeviceResources::brdf_lut() const {
    return _brdf_lut;
}

TextureView DeviceResources::white_noise() const {
    return _white_noise;
}

const AssetPtr<IBLProbe>& DeviceResources::ibl_probe() const {
    return _probe;
}

const AssetPtr<IBLProbe>& DeviceResources::empty_probe() const {
    return _empty_probe;
}

const ComputeProgram& DeviceResources::operator[](ComputePrograms i) const {
    y_debug_assert(usize(i) < usize(MaxComputePrograms));
    return _computes[usize(i)];
}

const MaterialTemplate* DeviceResources::operator[](MaterialTemplates i) const {
    y_debug_assert(usize(i) < usize(MaxMaterialTemplates));
    return &_material_templates[usize(i)];
}

const RaytracingProgram& DeviceResources::operator[](RaytracingPrograms i) const {
    y_debug_assert(_raytracing_programs);
    y_debug_assert(usize(i) < usize(MaxRaytracingPrograms));
    return _raytracing_programs[usize(i)];
}

const AssetPtr<Texture>& DeviceResources::operator[](Textures i) const {
    y_debug_assert(usize(i) < usize(MaxTextures));
    return _textures[usize(i)];
}

const AssetPtr<Material>& DeviceResources::operator[](Materials i) const {
    y_debug_assert(usize(i) < usize(MaxMaterials));
    return _materials[usize(i)];
}

const AssetPtr<StaticMesh>& DeviceResources::operator[](Meshes i) const {
    y_debug_assert(usize(i) < usize(MaxMeshes));
    return _meshes[usize(i)];
}

}

