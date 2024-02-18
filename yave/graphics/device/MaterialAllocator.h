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
#ifndef YAVE_GRAPHICS_DEVICE_MATERIALALLOCATOR_H
#define YAVE_GRAPHICS_DEVICE_MATERIALALLOCATOR_H

#include <yave/material/MaterialDrawData.h>

#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/buffers.h>

#include <yave/graphics/descriptors/uniforms.h>

#include <y/core/Vector.h>
#include <y/concurrent/Mutexed.h>


namespace yave {

class MaterialAllocator : NonMovable {
    public:
        static constexpr usize max_materials = 2 * 1024;

        MaterialAllocator();
        ~MaterialAllocator();

        MaterialDrawData allocate_material(const MaterialData& material);

        TypedSubBuffer<uniform::MaterialData, BufferUsage::StorageBit> material_buffer() const {
            return _materials;
        }

    private:
        friend class MaterialDrawData;

        void recycle(MaterialDrawData* data);

        TypedBuffer<uniform::MaterialData, BufferUsage::StorageBit, MemoryType::CpuVisible> _materials;
        concurrent::Mutexed<core::Vector<u32>> _free;

};

}

#endif // YAVE_GRAPHICS_DEVICE_MATERIALALLOCATOR_H

