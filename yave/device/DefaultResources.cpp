/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "DefaultResources.h"

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/material/Material.h>

#include <y/io/File.h>

namespace yave {

using SpirV = DefaultResources::SpirV;
using ComputePrograms = DefaultResources::ComputePrograms;
using Materials = DefaultResources::Materials;

struct DefaultMaterialData {
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
	};

static constexpr DefaultMaterialData material_datas[] = {
		{SpirV::BasicFrag, SpirV::BasicVert, true},
		{SpirV::SkinnedFrag, SpirV::SkinnedVert, true},
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

static constexpr usize spirv_count = usize(SpirV::MaxSpirV);
static constexpr usize compute_count = usize(ComputePrograms::MaxComputePrograms);
static constexpr usize material_count = usize(Materials::MaxMaterials);

static_assert(sizeof(spirv_names) / sizeof(spirv_names[0]) == spirv_count);
static_assert(sizeof(compute_spirvs) / sizeof(compute_spirvs[0]) == compute_count);
static_assert(sizeof(material_datas) / sizeof(material_datas[0]) == material_count);


static SpirVData load_spirv(const char* name) {
	return SpirVData::deserialized(io::File::open(fmt("%.spv", name)).expected("Unable to open SPIR-V file."));
}






DefaultResources::DefaultResources(DevicePtr dptr) :
		_spirv(std::make_unique<SpirVData[]>(spirv_count)),
		_computes(std::make_unique<ComputeProgram[]>(compute_count)),
		_materials(std::make_unique<AssetPtr<Material>[]>(material_count)) {

	for(usize i = 0; i != spirv_count; ++i) {
		_spirv[i] = load_spirv(spirv_names[i]);
	}

	for(usize i = 0; i != compute_count; ++i) {
		_computes[i] = ComputeProgram(ComputeShader(dptr, _spirv[usize(compute_spirvs[i])]));
	}

	for(usize i = 0; i != material_count; ++i) {
		const auto& data = material_datas[i];
		_materials[i] = make_asset<Material>(dptr, MaterialData()
				.set_frag_data(_spirv[data.frag])
				.set_vert_data(_spirv[data.vert])
				.set_depth_tested(data.depth_tested)
				.set_culled(data.culled)
				.set_blended(data.blended)
			);
	}
}

DefaultResources::DefaultResources() {
}

DefaultResources::~DefaultResources() {

}

DefaultResources::DefaultResources(DefaultResources&& other) {
	swap(other);
}

DefaultResources& DefaultResources::operator=(DefaultResources&& other) {
	swap(other);
	return *this;
}

void DefaultResources::swap(DefaultResources& other) {
	std::swap(_spirv, other._spirv);
	std::swap(_computes, other._computes);
	std::swap(_materials, other._materials);
}

const SpirVData& DefaultResources::operator[](SpirV i) const {
	return _spirv[usize(i)];
}

const ComputeProgram& DefaultResources::operator[](ComputePrograms i) const {
	return _computes[usize(i)];
}

const AssetPtr<Material>& DefaultResources::operator[](Materials i) const {
	return _materials[usize(i)];
}

}
