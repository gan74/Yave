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
#ifndef YAVE_GRAPHICS_SHADERS_SHADERMODULE_H
#define YAVE_GRAPHICS_SHADERS_SHADERMODULE_H

#include "ShaderModuleBase.h"

namespace yave {

template<ShaderType Type>
class ShaderModule : public ShaderModuleBase {

    static_assert(Type != ShaderType::None, "ShaderModule can not have None Type");

    public:
        ShaderModule() = default;

        ShaderModule(const SpirVData& data) : ShaderModuleBase(data) {
            if(type() != ShaderType::None && type() != Type) {
                y_fatal("Spirv data doesn't match ShaderModule Type.");
            }
        }
};

using FragmentShader = ShaderModule<ShaderType::Fragment>;
using VertexShader = ShaderModule<ShaderType::Vertex>;
using GeometryShader = ShaderModule<ShaderType::Geomery>;
using ComputeShader = ShaderModule<ShaderType::Compute>;

}

#endif // YAVE_GRAPHICS_SHADERS_SHADERMODULE_H

