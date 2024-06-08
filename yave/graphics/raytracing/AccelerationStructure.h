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
#ifndef YAVE_GRAPHICS_RAYTRACING_ACCELERATIONSTRUCTURE_H
#define YAVE_GRAPHICS_RAYTRACING_ACCELERATIONSTRUCTURE_H

#include <yave/graphics/buffers/Buffer.h>

#include <y/core/Span.h>

namespace yave {

using BLASInstance = VkAccelerationStructureInstanceKHR;

class AccelerationStructure {
    public:
        AccelerationStructure() = default;
        AccelerationStructure(AccelerationStructure&&) = default;
        AccelerationStructure& operator=(AccelerationStructure&&) = default;

        ~AccelerationStructure();

        bool is_null() const;

        SubBuffer<BufferUsage::AccelStructureBit> buffer() const;

        VkAccelerationStructureKHR vk_acc_struct() const;

    protected:
        VkHandle<VkAccelerationStructureKHR> _acc_struct;
        Buffer<BufferUsage::AccelStructureBit> _buffer;
};


class BLAS : public AccelerationStructure {
    public:
        BLAS() = default;

        BLAS(const MeshDrawData& mesh);
};

class TLAS : public AccelerationStructure {
    public:
        TLAS() = default;
        TLAS(core::Span<BLASInstance> instances);

        static BLASInstance make_instance(const math::Transform<> &tr, const BLAS &blas);
};

}

#endif // YAVE_GRAPHICS_RAYTRACING_ACCELERATIONSTRUCTURE_H

