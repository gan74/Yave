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
#ifndef YAVE_DEVICE_EXTENSIONS_RAYTRACING_H
#define YAVE_DEVICE_EXTENSIONS_RAYTRACING_H

#include <yave/graphics/graphics.h>
#include <yave/graphics/memory/DeviceMemory.h>

#include <y/core/Span.h>

namespace yave {

class RayTracing {
    public:
        class AccelerationStructure {
            public:
                AccelerationStructure() = default;

                AccelerationStructure(AccelerationStructure&&) = default;
                AccelerationStructure& operator=(AccelerationStructure&&) = default;

                AccelerationStructure(const StaticMesh& mesh);
                AccelerationStructure(core::Span<VkGeometryNV> geometries);

                ~AccelerationStructure();

            private:
                DeviceMemory _memory;
                VkHandle<VkAccelerationStructureNV> _acceleration_structure;
        };


        static const char* extension_name();

        RayTracing();

    private:
        friend class AccelerationStructure;

        PFN_vkCreateAccelerationStructureNV _create_acceleration_structure = nullptr;
        PFN_vkDestroyAccelerationStructureNV _destroy_acceleration_structure = nullptr;

        PFN_vkGetAccelerationStructureMemoryRequirementsNV _acceleration_structure_memory_reqs = nullptr;
        PFN_vkBindAccelerationStructureMemoryNV _bind_acceleration_structure_memory = nullptr;

        PFN_vkCmdTraceRaysNV _trace_rays = nullptr;

        /*PFN_vkGetAccelerationStructureMemoryRequirementsNV PFN_vkGetAccelerationStructureMemoryRequirementsNV = nullptr;
        PFN_vkBindAccelerationStructureMemoryNV PFN_vkBindAccelerationStructureMemoryNV = nullptr;
        PFN_vkCmdBuildAccelerationStructureNV PFN_vkCmdBuildAccelerationStructureNV = nullptr;
        PFN_vkCmdCopyAccelerationStructureNV PFN_vkCmdCopyAccelerationStructureNV = nullptr;
        PFN_vkCmdTraceRaysNV PFN_vkCmdTraceRaysNV = nullptr;
        PFN_vkCreateRayTracingPipelinesNV PFN_vkCreateRayTracingPipelinesNV = nullptr;
        PFN_vkGetRayTracingShaderGroupHandlesNV PFN_vkGetRayTracingShaderGroupHandlesNV = nullptr;
        PFN_vkGetAccelerationStructureHandleNV PFN_vkGetAccelerationStructureHandleNV = nullptr;
        PFN_vkCmdWriteAccelerationStructuresPropertiesNV PFN_vkCmdWriteAccelerationStructuresPropertiesNV = nullptr;
        PFN_vkCompileDeferredNV PFN_vkCompileDeferredNV = nullptr;*/
};

}

#endif // YAVE_DEVICE_EXTENSIONS_RAYTRACING_H

