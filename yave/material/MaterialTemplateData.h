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
#ifndef YAVE_MATERIAL_MATERIALTEMPLATEDATA_H
#define YAVE_MATERIAL_MATERIALTEMPLATEDATA_H

#include <yave/graphics/graphics.h>
#include <yave/graphics/shaders/SpirVData.h>

namespace yave {

enum class PrimitiveType {
    Triangles = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    Lines = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    Points = VK_PRIMITIVE_TOPOLOGY_POINT_LIST
};

enum class DepthTestMode {
    Standard,
    Reversed,
    None
};

enum class BlendMode {
    None,
    Add,
    SrcAlpha,
};

enum class CullMode {
    None,
    Back,
    Front,
};

class MaterialTemplateData {
    public:
        MaterialTemplateData& set_frag_data(const SpirVData& data);
        MaterialTemplateData& set_vert_data(const SpirVData& data);
        MaterialTemplateData& set_geom_data(const SpirVData& data);

        MaterialTemplateData& set_primitive_type(PrimitiveType type);

        MaterialTemplateData& set_depth_mode(DepthTestMode test);
        MaterialTemplateData& set_depth_write(bool write);

        MaterialTemplateData& set_blend_mode(BlendMode blend);

        MaterialTemplateData& set_cull_mode(CullMode cull);


    private:
        friend class MaterialCompiler;

        SpirVData _frag;
        SpirVData _vert;
        SpirVData _geom;

        PrimitiveType _primitive_type = PrimitiveType::Triangles;

        DepthTestMode _depth_mode = DepthTestMode::Standard;
        BlendMode _blend_mode = BlendMode::None;
        CullMode _cull_mode = CullMode::Back;

        bool _depth_write = true;
};

}

#endif // YAVE_MATERIAL_MATERIALTEMPLATEDATA_H

