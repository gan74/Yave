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

#include "DeviceResources.h"

#include <yave/graphics/device/extensions/DebugUtils.h>
#include <yave/graphics/shaders/SpirVData.h>
#include <yave/graphics/shaders/ShaderModule.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
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

using SpirV = DeviceResources::SpirV;
using ComputePrograms = DeviceResources::ComputePrograms;
using MaterialTemplates = DeviceResources::MaterialTemplates;
using Textures = DeviceResources::Textures;

struct DeviceMaterialData {
    const SpirV frag;
    const SpirV vert;
    const DepthTestMode depth_test;
    const BlendMode blend_mode;
    const CullMode cull_mode;
    const bool depth_write;

    static constexpr DeviceMaterialData screen(SpirV frag, bool blended = false) {
        return DeviceMaterialData{frag, SpirV::ScreenVert, DepthTestMode::None, blended ? BlendMode::Add : BlendMode::None, CullMode::None, false};
    }

    static constexpr DeviceMaterialData basic(SpirV frag) {
        return DeviceMaterialData{frag, SpirV::BasicVert, DepthTestMode::Standard, BlendMode::None,  CullMode::Back, true};
    }

    static constexpr DeviceMaterialData skinned(SpirV frag) {
        return DeviceMaterialData{frag, SpirV::SkinnedVert, DepthTestMode::Standard, BlendMode::None, CullMode::Back, true};
    }
};

static constexpr DeviceMaterialData material_datas[] = {
        DeviceMaterialData::basic(SpirV::TexturedFrag),
        DeviceMaterialData::basic(SpirV::TexturedAlphaFrag),
        DeviceMaterialData::basic(SpirV::TexturedAlphaEmissiveFrag),
        DeviceMaterialData::skinned(SpirV::TexturedFrag),
        DeviceMaterialData{SpirV::DeferredPointFrag, SpirV::DeferredPointVert, DepthTestMode::Reversed, BlendMode::Add, CullMode::Front, false},
        DeviceMaterialData{SpirV::DeferredSpotFrag, SpirV::DeferredSpotVert, DepthTestMode::Reversed, BlendMode::Add, CullMode::Front, false},
        DeviceMaterialData::screen(SpirV::DeferredAmbientFrag, true),
        DeviceMaterialData::screen(SpirV::AtmosphereFrag, true),
        DeviceMaterialData::screen(SpirV::AtmosphereIntegrationFrag),
        DeviceMaterialData::screen(SpirV::ToneMapFrag),
        DeviceMaterialData::screen(SpirV::PassthroughFrag),
        DeviceMaterialData::screen(SpirV::PassthroughFrag, true),
        DeviceMaterialData::screen(SpirV::DownsampleFrag),
        DeviceMaterialData::screen(SpirV::BloomFrag, true),
        DeviceMaterialData::screen(SpirV::BloomCombineFrag),
        DeviceMaterialData::screen(SpirV::HBlurFrag, true),
        DeviceMaterialData::screen(SpirV::VBlurFrag, true),
    };

static constexpr const char* spirv_names[] = {
        "equirec_convolution.comp",
        "cubemap_convolution.comp",
        "brdf_integrator.comp",
        "deferred_locals.comp",
        "linearize_depth.comp",
        "ssao.comp",
        "ssao_upsample.comp",
        "ssao_upsample_merge.comp",
        "copy.comp",

        "histogram_clear.comp",
        "histogram.comp",
        "tonemap_params.comp",
        "depth_bounds.comp",

        "deferred_point.frag",
        "deferred_spot.frag",
        "deferred_ambient.frag",
        "atmosphere.frag",
        "atmosphere_integrator.frag",
        "tonemap.frag",
        "textured.frag",
        "textured_alpha.frag",
        "textured_alpha_emissive.frag",
        "passthrough.frag",
        "downsample.frag",
        "bloom.frag",
        "bloom_combine.frag",
        "hblur.frag",
        "vblur.frag",

        "deferred_point.vert",
        "deferred_spot.vert",
        "basic.vert",
        "skinned.vert",
        "screen.vert",
    };

// ABGR
static constexpr std::array<u32, 6> texture_colors[] = {
        {0, 0, 0, 0},
        {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
        {0xFF7F7F7F, 0xFF7F7F7F, 0xFF7F7F7F, 0xFF7F7F7F},
        {0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF},
        {0x00FF7F7F, 0x00FF7F7F, 0x00FF7F7F, 0x00FF7F7F},
        {0xFFEBCE87, 0xFFEBCE87, 0xFF272E3A, 0xFF272E3A}
    };

static constexpr usize spirv_count = usize(SpirV::MaxSpirV);
static constexpr usize compute_count = usize(ComputePrograms::MaxComputePrograms);
static constexpr usize template_count = usize(MaterialTemplates::MaxMaterialTemplates);
static constexpr usize texture_count = usize(Textures::MaxTextures);

static_assert(sizeof(spirv_names) / sizeof(spirv_names[0]) == spirv_count);
static_assert(sizeof(material_datas) / sizeof(material_datas[0]) == template_count);
static_assert(sizeof(texture_colors) / sizeof(texture_colors[0]) == texture_count);

// implemented in DeviceResourcesData.cpp
MeshData cube_mesh_data();
MeshData sphere_mesh_data();
MeshData simple_sphere_mesh_data();
MeshData cone_mesh_data();
MeshData sweep_mesh_data();


static Texture create_brdf_lut(const ComputeProgram& brdf_integrator, usize size = 512) {
    y_profile();
    core::DebugTimer _("create_ibl_lut()");

    StorageTexture image(ImageFormat(VK_FORMAT_R16G16_UNORM), {size, size});

    const DescriptorSet dset = DescriptorSet(Descriptor(StorageView(image)));

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    {
        const auto region = recorder.region("create_brdf_lut");
        recorder.dispatch_size(brdf_integrator, image.size(), dset);
    }
    std::move(recorder).submit<SyncPolicy::Sync>();

    return image;
}

static Texture create_white_noise(usize size = 256) {
    y_profile();
    core::DebugTimer _("create_white_noise()");

    std::unique_ptr<u32[]> data = std::make_unique<u32[]>(size * size);

    math::FastRandom rng;
    std::generate_n(data.get(), size * size, rng);

    return Texture(ImageData(math::Vec2ui(size, size), reinterpret_cast<const u8*>(data.get()), VK_FORMAT_R8G8B8A8_UNORM));
}



DeviceResources::DeviceResources() {
    y_profile();

    _spirv = std::make_unique<SpirVData[]>(spirv_count);
    _computes = std::make_unique<ComputeProgram[]>(compute_count);
    _material_templates = std::make_unique<MaterialTemplate[]>(template_count);
    _textures = std::make_unique<AssetPtr<Texture>[]>(texture_count);

    const auto set_name = [debug = debug_utils()](auto handle, const char* name) {
        if(debug) {
            debug->set_resource_name(handle, name);
        }
    };

    // Load textures here because they won't change and might get packed into descriptor sets that won't be reloaded (in the case of material defaults)
    for(usize i = 0; i != texture_count; ++i) {
        const u8* data = reinterpret_cast<const u8*>(texture_colors[i].data());
        _textures[i] = make_asset<Texture>(ImageData(math::Vec2ui(2), data, VK_FORMAT_R8G8B8A8_UNORM));

        set_name(_textures[i]->vk_image(), "Resource Image");
        set_name(_textures[i]->vk_view(), "Resource Image View");
    }

    for(usize i = 0; i != spirv_count; ++i) {
        const auto file_name = fmt("%.spv", spirv_names[i]);
        _spirv[i] = SpirVData::deserialized(io2::File::open(file_name).expected(fmt_c_str("Unable to open SPIR-V file (%).", file_name)));
    }

    for(usize i = 0; i != compute_count; ++i) {
        _computes[i] = ComputeProgram(ComputeShader(_spirv[i]));
        set_name(_computes[i].vk_pipeline(), spirv_names[i]);
    }

    for(usize i = 0; i != template_count; ++i) {
        const auto& data = material_datas[i];
        auto template_data = MaterialTemplateData()
                .set_frag_data(_spirv[data.frag])
                .set_vert_data(_spirv[data.vert])
                .set_depth_mode(data.depth_test)
                .set_cull_mode(data.cull_mode)
                .set_blend_mode(data.blend_mode)
                .set_depth_write(data.depth_write);
            ;
        _material_templates[i] = MaterialTemplate(std::move(template_data));
        _material_templates[i].set_name(spirv_names[data.frag]);
    }

    {
        _materials = std::make_unique<AssetPtr<Material>[]>(usize(MaxMaterials));
        _materials[0] = make_asset<Material>(&_material_templates[usize(TexturedMaterialTemplate)]);
    }

    {
        _meshes = std::make_unique<AssetPtr<StaticMesh>[]>(usize(MaxMeshes));
        _meshes[0] = make_asset<StaticMesh>(cube_mesh_data());
        _meshes[1] = make_asset<StaticMesh>(sphere_mesh_data());
        _meshes[2] = make_asset<StaticMesh>(simple_sphere_mesh_data());
        _meshes[3] = make_asset<StaticMesh>(cone_mesh_data());
        _meshes[4] = make_asset<StaticMesh>(sweep_mesh_data());
    }


    _brdf_lut = create_brdf_lut(operator[](BRDFIntegratorProgram));
    set_name(_brdf_lut.vk_image(), "BRDF LUT");
    set_name(_brdf_lut.vk_view(), "BRDF LUT View");

    _white_noise = create_white_noise();
    set_name(_white_noise.vk_image(), "White Noise");
    set_name(_white_noise.vk_view(), "White Noise View");

    _probe = make_asset<IBLProbe>(IBLProbe::from_equirec(*operator[](SkyIBLTexture)));
    set_name(_probe->vk_image(), "Default Probe");
    set_name(_probe->vk_view(), "Default Probe");

    _empty_probe = make_asset<IBLProbe>(IBLProbe::from_equirec(*operator[](BlackTexture)));
    set_name(_empty_probe->vk_image(), "Empty Probe");
    set_name(_empty_probe->vk_view(), "Empty Probe");
}


DeviceResources::~DeviceResources() {
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

const SpirVData& DeviceResources::operator[](SpirV i) const {
    y_debug_assert(usize(i) < usize(MaxSpirV));
    return _spirv[usize(i)];
}

const ComputeProgram& DeviceResources::operator[](ComputePrograms i) const {
    y_debug_assert(usize(i) < usize(MaxComputePrograms));
    return _computes[usize(i)];
}

const MaterialTemplate* DeviceResources::operator[](MaterialTemplates i) const {
    y_debug_assert(usize(i) < usize(MaxMaterialTemplates));
    return &_material_templates[usize(i)];
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

