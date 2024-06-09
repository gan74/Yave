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

#include "AccelerationStructure.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/meshes/MeshDrawData.h>


namespace yave {

static VkDeviceOrHostAddressConstKHR buffer_device_address(const SubBufferBase& buffer) {
    VkBufferDeviceAddressInfo info = vk_struct();
    info.buffer = buffer.vk_buffer();

    VkDeviceOrHostAddressConstKHR addr = {};
    addr.deviceAddress = vkGetBufferDeviceAddress(vk_device(), &info) + buffer.byte_offset();

    return addr;
}



static std::pair<VkHandle<VkAccelerationStructureKHR>, Buffer<BufferUsage::AccelStructureBit>> create_acceleration_structure(
        core::Span<VkAccelerationStructureGeometryKHR> geometries,
        core::Span<u32> prim_counts,
        core::Span<VkAccelerationStructureBuildRangeInfoKHR> ranges,
        VkAccelerationStructureTypeKHR struct_type) {

    y_profile();

    y_debug_assert(geometries.size() == prim_counts.size());
    y_debug_assert(geometries.size() == ranges.size());

    VkAccelerationStructureBuildGeometryInfoKHR build_info = vk_struct();
    {
        build_info.type = struct_type;
        build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.geometryCount = u32(geometries.size());
        build_info.pGeometries = geometries.data();
    }

    VkAccelerationStructureBuildSizesInfoKHR size_infos = vk_struct();
    vkGetAccelerationStructureBuildSizesKHR(vk_device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, prim_counts.data(), &size_infos);

    Buffer<BufferUsage::AccelStructureBit> buffer(size_infos.accelerationStructureSize);
    Buffer<BufferUsage::StorageBit> scratch(size_infos.buildScratchSize);

    build_info.scratchData.deviceAddress = buffer_device_address(SubBuffer(scratch)).deviceAddress;

    VkAccelerationStructureCreateInfoKHR create_info = vk_struct();
    {
        create_info.type = struct_type;
        create_info.buffer = buffer.vk_buffer();
        create_info.size = buffer.byte_size();
    }

    VkHandle<VkAccelerationStructureKHR> blas;
    vk_check(vkCreateAccelerationStructureKHR(vk_device(), &create_info, vk_allocation_callbacks(), blas.get_ptr_for_init()));
    build_info.dstAccelerationStructure = blas;


    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    {
        const auto region = recorder.region("Build acceleration structurte");
        const VkAccelerationStructureBuildRangeInfoKHR* build_range_ptr = ranges.data();
        vkCmdBuildAccelerationStructuresKHR(recorder.vk_cmd_buffer(), 1, &build_info, &build_range_ptr);
    }
    recorder.submit();

    return {std::move(blas), std::move(buffer)};
}



static std::pair<VkHandle<VkAccelerationStructureKHR>, Buffer<BufferUsage::AccelStructureBit>> create_blas(const MeshDrawData& mesh) {
    const MeshDrawBuffers& mesh_buffers = mesh.mesh_buffers();
    const MeshDrawCommand& draw_params = mesh.draw_command();

    const u32 prim_count = draw_params.index_count / 3;
    y_debug_assert(prim_count * 3 == draw_params.index_count);

    VkAccelerationStructureGeometryTrianglesDataKHR triangles = vk_struct();
    {
        const AttribSubBuffer& position_buffer = mesh_buffers.attrib_buffers()[usize(VertexStreamType::Position)];

        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexStride = sizeof(math::Vec3);
        triangles.vertexData = buffer_device_address(position_buffer);
        triangles.maxVertex = u32(mesh_buffers.vertex_count());

        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData = buffer_device_address(mesh_buffers.triangle_buffer());
    }

    VkAccelerationStructureGeometryKHR geometry = vk_struct();
    {
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.triangles = triangles;
    }

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    {
        range.primitiveCount = prim_count;
        range.primitiveOffset = u32(mesh.triangle_buffer().byte_offset()) + (draw_params.first_index * sizeof(u32));
        range.firstVertex = draw_params.vertex_offset;
    }

    return create_acceleration_structure(geometry, prim_count, range, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
}





static std::pair<VkHandle<VkAccelerationStructureKHR>, Buffer<BufferUsage::AccelStructureBit>> create_tlas(core::Span<VkAccelerationStructureInstanceKHR> instances) {
    TypedBuffer<VkAccelerationStructureInstanceKHR, BufferUsage::AccelStructureInputBit, MemoryType::CpuVisible> instance_buffer(instances.size());
    {
        auto mapping = instance_buffer.map(MappingAccess::WriteOnly);
        std::copy(instances.begin(), instances.end(), mapping.begin());
    }

    VkAccelerationStructureGeometryInstancesDataKHR geo_instances = vk_struct();
    {
        geo_instances.data = buffer_device_address(SubBuffer(instance_buffer));
    }

    VkAccelerationStructureGeometryKHR geometry = vk_struct();
    {
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.instances = geo_instances;
    }

    const u32 prim_count = u32(instances.size());

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    {
        range.primitiveCount = prim_count;
    }

    return create_acceleration_structure(geometry, prim_count, range, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
}




AccelerationStructure::~AccelerationStructure() {
    destroy_graphic_resource(std::move(_acc_struct));
}

bool AccelerationStructure::is_null() const {
    return _acc_struct.is_null();
}

SubBuffer<BufferUsage::AccelStructureBit> AccelerationStructure::buffer() const {
    return _buffer;
}

VkAccelerationStructureKHR AccelerationStructure::vk_accel_struct() const {
    return _acc_struct.get();
}

VkDeviceAddress AccelerationStructure::vk_device_address() const {
    return _address;
}


BLAS::BLAS(const MeshDrawData& mesh) {
    std::tie(_acc_struct, _buffer) = create_blas(mesh);

    VkAccelerationStructureDeviceAddressInfoKHR addr_info = vk_struct();
    addr_info.accelerationStructure = vk_accel_struct();
    _address = vkGetAccelerationStructureDeviceAddressKHR(vk_device(), &addr_info);
}


TLAS::TLAS(core::Span<BLASInstance> instances) {
    std::tie(_acc_struct, _buffer) = create_tlas(instances);

    VkAccelerationStructureDeviceAddressInfoKHR addr_info = vk_struct();
    addr_info.accelerationStructure = vk_accel_struct();
    _address = vkGetAccelerationStructureDeviceAddressKHR(vk_device(), &addr_info);
}

BLASInstance TLAS::make_instance(const math::Transform<>& tr, const BLAS& blas) {
    BLASInstance instance = {};

    {
        instance.mask = 0xFF;
        instance.accelerationStructureReference = blas.vk_device_address();

        const math::Matrix mat = tr.matrix().to<3, 4>().transposed();
        std::copy(mat.begin(), mat.end(), &instance.transform.matrix[0][0]);
    }

    return instance;
}

}

