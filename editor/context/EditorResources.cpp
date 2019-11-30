/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "EditorResources.h"

#include <yave/graphics/shaders/SpirVData.h>
#include <yave/graphics/shaders/ShaderModule.h>
#include <yave/graphics/shaders/ComputeProgram.h>

#include <yave/material/MaterialTemplate.h>

#include <y/io2/File.h>

namespace editor {

using SpirV = EditorResources::SpirV;
using ComputePrograms = EditorResources::ComputePrograms;
using MaterialTemplates = EditorResources::MaterialTemplates;


static constexpr const char* spirv_names[] = {
		"depth_alpha.comp",
		"picking.comp",

		"imgui.frag",
		"imgui_billboard.frag",
		"basic_picking.frag",
		"copy_target.frag",

		"screen.vert",
		"imgui.vert",
		"imgui_billboard.vert",
		"basic_picking.vert",

		"imgui_billboard.geom",
	};

struct DeviceMaterialData {
	const SpirV frag;
	const SpirV vert;
	const SpirV geom = SpirV::MaxSpirV;
	const bool depth_tested;
	const bool culled = true;
	const bool blended = false;
	const PrimitiveType prim_type = PrimitiveType::Triangles;
};


static constexpr DeviceMaterialData material_datas[] = {
		{SpirV::ImGuiFrag,			SpirV::ImGuiVert,			SpirV::MaxSpirV,			false,	false,	true,	PrimitiveType::Triangles},
		{SpirV::ImGuiBillBoardFrag, SpirV::ImGuiBillBoardVert,	SpirV::ImGuiBillBoardGeom,	true,	false,	true,	PrimitiveType::Points},
		{SpirV::PickingFrag,		SpirV::PickingVert,			SpirV::MaxSpirV,			true,	true,	true,	PrimitiveType::Triangles},
		{SpirV::PickingFrag,		SpirV::ImGuiBillBoardVert,	SpirV::ImGuiBillBoardGeom,	true,	false,	true,	PrimitiveType::Points},
		{SpirV::CopyTargetFrag,		SpirV::ScreenVert,			SpirV::MaxSpirV,			false,	false,	true,	PrimitiveType::Triangles},
	};


static constexpr usize spirv_count = usize(SpirV::MaxSpirV);
static constexpr usize compute_count = usize(ComputePrograms::MaxComputePrograms);
static constexpr usize template_count = usize(MaterialTemplates::MaxMaterialTemplates);

static_assert(sizeof(spirv_names) / sizeof(spirv_names[0]) == spirv_count);
static_assert(sizeof(material_datas) / sizeof(material_datas[0]) == template_count);


EditorResources::EditorResources(DevicePtr dptr) :
		_spirv(std::make_unique<SpirVData[]>(spirv_count)),
		_computes(std::make_unique<ComputeProgram[]>(compute_count)),
		_material_templates(std::make_unique<MaterialTemplate[]>(template_count)) {

	load_resources(dptr);
}

EditorResources::~EditorResources() {
}

void EditorResources::load_resources(DevicePtr dptr) {
	for(usize i = 0; i != spirv_count; ++i) {
		_spirv[i] = SpirVData::deserialized(io2::File::open(fmt("%.spv", spirv_names[i])).expected("Unable to open SPIR-V file."));
	}

	for(usize i = 0; i != compute_count; ++i) {
		_computes[i] = ComputeProgram(ComputeShader(dptr, _spirv[i]));
	}

	for(usize i = 0; i != template_count; ++i) {
		const auto& data = material_datas[i];
		auto template_data = MaterialTemplateData()
				.set_frag_data(_spirv[data.frag])
				.set_vert_data(_spirv[data.vert])
				.set_depth_test(data.depth_tested ? DepthTestMode::Standard : DepthTestMode::None)
				.set_cull_mode(data.culled ? CullMode::Back : CullMode::None)
				.set_blend_mode(data.blended ? BlendMode::SrcAlpha : BlendMode::None)
				.set_primitive_type(data.prim_type)
			;

		if(data.geom != SpirV::MaxSpirV) {
			template_data.set_geom_data(_spirv[data.geom]);
		}

		_material_templates[i] = MaterialTemplate(dptr, std::move(template_data));
	}
}

DevicePtr EditorResources::device() const {
	return _computes[0].device();
}

const ComputeProgram& EditorResources::operator[](ComputePrograms i) const {
	y_debug_assert(usize(i) < usize(MaxComputePrograms));
	return _computes[usize(i)];
}

const MaterialTemplate* EditorResources::operator[](MaterialTemplates i) const {
	y_debug_assert(usize(i) < usize(MaxMaterialTemplates));
	return &_material_templates[usize(i)];
}

void EditorResources::reload() {
	y_profile();
	DevicePtr dptr = device();
	dptr->wait_all_queues();
	load_resources(dptr);
}

}
