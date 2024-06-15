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

#include "EditorResources.h"

#include <yave/graphics/shaders/SpirVData.h>
#include <yave/graphics/shaders/ShaderModule.h>
#include <yave/graphics/shaders/ComputeProgram.h>

#include <yave/material/MaterialTemplate.h>

#include <y/io2/File.h>

#include <y/utils/format.h>

namespace editor {

using ComputePrograms = EditorResources::ComputePrograms;
using MaterialTemplates = EditorResources::MaterialTemplates;

struct DeviceMaterialData {
    const std::string_view frag;
    const std::string_view vert;
    const std::string_view geom;
    const bool depth_tested;
    const bool culled = true;
    const bool blended = false;
    const PrimitiveType prim_type = PrimitiveType::Triangles;
};



static constexpr std::array<DeviceMaterialData, usize(MaterialTemplates::MaxMaterialTemplates)> material_datas = {
    DeviceMaterialData{"imgui",             "imgui",            {},                 false,  false,  true,   PrimitiveType::Triangles},
    DeviceMaterialData{"imgui_billboard",   "imgui_billboard",  "imgui_billboard",  true,   false,  false,  PrimitiveType::Points},
    DeviceMaterialData{"engine_view",       "screen",           {},                 false,  false,  false,  PrimitiveType::Triangles},
    DeviceMaterialData{"selection",         "screen",           {},                 false,  false,  true,   PrimitiveType::Triangles},
};

static constexpr std::array<std::string_view, usize(ComputePrograms::MaxComputePrograms)> compute_datas = {
    "depth_alpha",
    "picking",
};


EditorResources::EditorResources() :
        _computes(std::make_unique<ComputeProgram[]>(compute_datas.size())),
        _material_templates(std::make_unique<MaterialTemplate[]>(material_datas.size())) {

    load_resources();
}

EditorResources::~EditorResources() {
}

void EditorResources::load_resources() {
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

    for(usize i = 0; i != compute_datas.size(); ++i) {
        _computes[i] = ComputeProgram(ComputeShader(load_spirv(compute_datas[i])));
    }

    for(usize i = 0; i != material_datas.size(); ++i) {
        const auto& data = material_datas[i];
        auto template_data = MaterialTemplateData()
            .set_frag_data(load_spirv(data.frag))
            .set_vert_data(load_spirv(data.vert))
            .set_depth_mode(data.depth_tested ? DepthTestMode::Standard : DepthTestMode::None)
            .set_cull_mode(data.culled ? CullMode::Back : CullMode::None)
            .set_blend_mode(data.blended ? BlendMode::SrcAlpha : BlendMode::None)
            .set_primitive_type(data.prim_type)
        ;

        if(!data.geom.empty()) {
            template_data.set_geom_data(load_spirv(data.geom));
        }

        _material_templates[i] = MaterialTemplate(std::move(template_data));
    }
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
    y_profile();;
    wait_all_queues();
    load_resources();
}

}

