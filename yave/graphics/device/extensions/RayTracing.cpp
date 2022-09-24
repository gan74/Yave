/*******************************
Copyright (c) 2016-2022 Gr�goire Angerand

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

#include "RayTracing.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>

#include <yave/meshes/StaticMesh.h>

namespace yave {

RayTracing::AccelerationStructure::AccelerationStructure(const StaticMesh& mesh) : AccelerationStructure(create_mesh_geometry(mesh)) {
}

RayTracing::AccelerationStructure::AccelerationStructure(core::Span<VkGeometryNV> geometries) {
    y_profile();

    const RayTracing* rt = ray_tracing();
    y_debug_assert(rt);

    {
        VkAccelerationStructureInfoNV as_info = vk_struct();
        as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
        as_info.geometryCount = u32(geometries.size());
        as_info.pGeometries = geometries.data();

        VkAccelerationStructureCreateInfoNV create_info = vk_struct();
        create_info.info = as_info;
        vk_check(rt->_create_acceleration_structure(vk_device(), &create_info, vk_allocation_callbacks(), &_acceleration_structure.get()));
    }

    {
        VkAccelerationStructureMemoryRequirementsInfoNV mem_reqs_info = vk_struct();
        mem_reqs_info.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
        mem_reqs_info.accelerationStructure = _acceleration_structure;

        VkMemoryRequirements2 mem_reqs = vk_struct();
        rt->_acceleration_structure_memory_reqs(vk_device(), &mem_reqs_info, &mem_reqs);

        _memory = device_allocator().alloc(mem_reqs.memoryRequirements, MemoryType::DeviceLocal);
    }

    {
        VkBindAccelerationStructureMemoryInfoNV bind_info = vk_struct();
        bind_info.accelerationStructure = _acceleration_structure;
        bind_info.memory = _memory.vk_memory();
        vk_check(rt->_bind_acceleration_structure_memory(vk_device(), 1, &bind_info));
    }
}

RayTracing::AccelerationStructure::~AccelerationStructure() {
    const RayTracing* rt = ray_tracing();
    y_debug_assert(rt);

    rt->_destroy_acceleration_structure(vk_device(), _acceleration_structure, vk_allocation_callbacks());

    destroy_graphic_resource(std::move(_memory));
}

VkGeometryNV RayTracing::AccelerationStructure::create_mesh_geometry(const StaticMesh& mesh) {
    VkGeometryNV geometry = vk_struct();

    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

    {
        geometry.geometry.triangles = vk_struct();
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;

        const auto& draw_data = mesh.draw_data();
        const auto& draw_command = draw_data.draw_command();

        const auto& position_buffer = draw_data.position_buffer();
        geometry.geometry.triangles.vertexData = position_buffer.vk_buffer();
        geometry.geometry.triangles.vertexOffset = draw_command.vertex_offset;
        geometry.geometry.triangles.vertexCount = u32(position_buffer.size()); Y_TODO(This is wrong!)
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexStride = position_buffer.element_size();

        const auto& triangle_buffer = draw_data.triangle_buffer();
        geometry.geometry.triangles.indexData = triangle_buffer.vk_buffer();
        geometry.geometry.triangles.indexOffset = draw_command.first_index;
        geometry.geometry.triangles.indexCount = draw_command.index_count;
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    }
    {
        geometry.geometry.aabbs = vk_struct();
    }

    return geometry;
}






const char* RayTracing::extension_name() {
    return VK_NV_RAY_TRACING_EXTENSION_NAME;
}

RayTracing::RayTracing() {

#define GET_PROC(name) reinterpret_cast<PFN_ ## name>(vkGetDeviceProcAddr(vk_device(), #name));

    _create_acceleration_structure = GET_PROC(vkCreateAccelerationStructureNV);
    _destroy_acceleration_structure = GET_PROC(vkDestroyAccelerationStructureNV);

    _acceleration_structure_memory_reqs = GET_PROC(vkGetAccelerationStructureMemoryRequirementsNV);
    _bind_acceleration_structure_memory = GET_PROC(vkBindAccelerationStructureMemoryNV);

    _trace_rays = GET_PROC(vkCmdTraceRaysNV);

#undef GET_PROC

}

}

