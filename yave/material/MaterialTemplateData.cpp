/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "MaterialTemplateData.h"

#include <yave/graphics/shaders/ShaderModule.h>

namespace yave {

MaterialTemplateData& MaterialTemplateData::set_frag_data(const SpirVData& data) {
    y_debug_assert(ShaderModuleBase::shader_type(data) == ShaderType::Fragment);
    _frag = data;
    return *this;
}

MaterialTemplateData& MaterialTemplateData::set_vert_data(const SpirVData& data) {
    y_debug_assert(ShaderModuleBase::shader_type(data) == ShaderType::Vertex);
    _vert = data;
    return *this;
}

MaterialTemplateData& MaterialTemplateData::set_geom_data(const SpirVData& data) {
    y_debug_assert(ShaderModuleBase::shader_type(data) == ShaderType::Geomery);
    _geom = data;
    return *this;
}

MaterialTemplateData& MaterialTemplateData::set_primitive_type(PrimitiveType type) {
    _primitive_type = type;
    return *this;
}

MaterialTemplateData& MaterialTemplateData::set_depth_mode(DepthTestMode test) {
    _depth_mode = test;
    return *this;
}

MaterialTemplateData& MaterialTemplateData::set_depth_write(bool write) {
    _depth_write = write;
    return *this;
}

MaterialTemplateData& MaterialTemplateData::set_blend_mode(BlendMode blend) {
    _blend_mode = blend;
    return *this;
}

MaterialTemplateData& MaterialTemplateData::set_cull_mode(CullMode cull) {
    _cull_mode = cull;
    return *this;
}


}

