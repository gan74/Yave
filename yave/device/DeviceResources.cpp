/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include <yave/graphics/shaders/SpirVData.h>
#include <yave/graphics/shaders/ShaderModule.h>
#include <yave/graphics/shaders/ComputeProgram.h>

#include <yave/material/Material.h>

#include <yave/meshes/MeshData.h>
#include <yave/meshes/StaticMesh.h>

#include <y/io2/File.h>

namespace yave {

using SpirV = DeviceResources::SpirV;
using ComputePrograms = DeviceResources::ComputePrograms;
using MaterialTemplates = DeviceResources::MaterialTemplates;
using Textures = DeviceResources::Textures;

struct DeviceMaterialData {
	SpirV frag;
	SpirV vert;
	bool depth_tested;
	bool culled = true;
	bool blended = false;
};

static constexpr SpirV compute_spirvs[] = {
		SpirV::EquirecConvolutionComp,
		SpirV::CubemapConvolutionComp,
		SpirV::BRDFIntegratorComp,
		SpirV::DeferredLightingComp,
		SpirV::SSAOComp,
		SpirV::PickingComp,
		SpirV::DepthAlphaComp,
		SpirV::CopyComp,
	};

static constexpr DeviceMaterialData material_datas[] = {
		{SpirV::BasicFrag, SpirV::BasicVert, true},
		{SpirV::SkinnedFrag, SpirV::SkinnedVert, true},

		{SpirV::TexturedFrag, SpirV::BasicVert, true},

		{SpirV::TonemapFrag, SpirV::ScreenVert, false},
		{SpirV::ImguiFrag, SpirV::ImguiVert, false, false, true},
	};

static constexpr const char* spirv_names[] = {
		"equirec_convolution.comp",
		"cubemap_convolution.comp",
		"brdf_integrator.comp",
		"deferred.comp",
		"ssao.comp",
		"picking.comp",
		"depth_alpha.comp",
		"copy.comp",

		"tonemap.frag",
		"basic.frag",
		"skinned.frag",
		"textured.frag",
		"imgui.frag",

		"basic.vert",
		"skinned.vert",
		"screen.vert",
		"imgui.vert",
	};

static constexpr std::array<u32, 4> texture_colors[] = {
		{0, 0, 0, 0},
		{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
		{0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF},
		{0x00FF7F7F, 0x00FF7F7F, 0x00FF7F7F, 0x00FF7F7F}
	};

static constexpr usize spirv_count = usize(SpirV::MaxSpirV);
static constexpr usize compute_count = usize(ComputePrograms::MaxComputePrograms);
static constexpr usize material_count = usize(MaterialTemplates::MaxMaterialTemplates);
static constexpr usize texture_count = usize(Textures::MaxTextures);

static_assert(sizeof(spirv_names) / sizeof(spirv_names[0]) == spirv_count);
static_assert(sizeof(compute_spirvs) / sizeof(compute_spirvs[0]) == compute_count);
static_assert(sizeof(material_datas) / sizeof(material_datas[0]) == material_count);
static_assert(sizeof(texture_colors) / sizeof(texture_colors[0]) == texture_count);

static SpirVData load_spirv(const char* name) {
	return SpirVData::deserialized(io2::File::open(fmt("%.spv", name)).expected("Unable to open SPIR-V file."));
}

// implemented in DeviceResourcesData.cpp
MeshData cube_mesh_data();
MeshData sphere_mesh_data();


DeviceResources::DeviceResources(DevicePtr dptr) :
		_spirv(std::make_unique<SpirVData[]>(spirv_count)),
		_computes(std::make_unique<ComputeProgram[]>(compute_count)),
		_material_templates(std::make_unique<MaterialTemplate[]>(material_count)) ,
		_textures(std::make_unique<AssetPtr<Texture>[]>(texture_count)) {

	for(usize i = 0; i != spirv_count; ++i) {
		_spirv[i] = load_spirv(spirv_names[i]);
	}

	for(usize i = 0; i != compute_count; ++i) {
		_computes[i] = ComputeProgram(ComputeShader(dptr, _spirv[usize(compute_spirvs[i])]));
	}

	for(usize i = 0; i != material_count; ++i) {
		const auto& data = material_datas[i];
		auto template_data = MaterialTemplateData()
				.set_frag_data(_spirv[data.frag])
				.set_vert_data(_spirv[data.vert])
				.set_depth_tested(data.depth_tested)
				.set_culled(data.culled)
				.set_blended(data.blended)
			;
		_material_templates[i] = MaterialTemplate(dptr, std::move(template_data));
	}

	for(usize i = 0; i != texture_count; ++i) {
		const u8* data = reinterpret_cast<const u8*>(texture_colors[i].data());
		_textures[i] = make_asset<Texture>(dptr, ImageData(math::Vec2ui(2), data, vk::Format::eR8G8B8A8Unorm));
	}


	{
		_materials = std::make_unique<AssetPtr<Material>[]>(1);
		_materials[0] = make_asset<Material>(&_material_templates[usize(BasicMaterialTemplate)]);
	}

	{
		_meshes = std::make_unique<AssetPtr<StaticMesh>[]>(2);
		_meshes[0] = make_asset<StaticMesh>(dptr, cube_mesh_data());
		_meshes[1] = make_asset<StaticMesh>(dptr, sphere_mesh_data());
	}
}

DeviceResources::DeviceResources() {
}

DeviceResources::~DeviceResources() {

}

DeviceResources::DeviceResources(DeviceResources&& other) {
	swap(other);
}

DeviceResources& DeviceResources::operator=(DeviceResources&& other) {
	swap(other);
	return *this;
}

void DeviceResources::swap(DeviceResources& other) {
	std::swap(_spirv, other._spirv);
	std::swap(_computes, other._computes);
	std::swap(_material_templates, other._material_templates);
	std::swap(_textures, other._textures);
	std::swap(_materials, other._materials);
	std::swap(_meshes, other._meshes);
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
