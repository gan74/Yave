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



static std::pair<VkHandle<VkAccelerationStructureKHR>, Buffer<BufferUsage::AccelStructureBit>> create_acceleration_structure(const VkAccelerationStructureGeometryDataKHR& input_geo, VkAccelerationStructureTypeKHR struct_type) {
    y_profile();

    VkAccelerationStructureGeometryKHR geometry = vk_struct();
    {
        geometry.geometryType = input_geo.triangles.indexData.deviceAddress ? VK_GEOMETRY_TYPE_TRIANGLES_KHR : VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry = input_geo;
    }

    VkAccelerationStructureBuildGeometryInfoKHR geometry_infos = vk_struct();
    {
        geometry_infos.type = struct_type;
        geometry_infos.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        geometry_infos.geometryCount = 1;
        geometry_infos.pGeometries = &geometry;
    }

    const u32 primitive_count = 1;
    VkAccelerationStructureBuildSizesInfoKHR size_infos = vk_struct();
    vkGetAccelerationStructureBuildSizesKHR(vk_device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &geometry_infos, &primitive_count, &size_infos);

    Buffer<BufferUsage::AccelStructureBit> buffer(size_infos.accelerationStructureSize);
    Buffer<BufferUsage::StorageBit> scratch(size_infos.buildScratchSize);

    VkAccelerationStructureCreateInfoKHR create_info = vk_struct();
    {
        create_info.type = struct_type;
        create_info.buffer = buffer.vk_buffer();
        create_info.size = buffer.byte_size();
    }

    VkHandle<VkAccelerationStructureKHR> blas;
    vk_check(vkCreateAccelerationStructureKHR(vk_device(), &create_info, vk_allocation_callbacks(), blas.get_ptr_for_init()));

    VkAccelerationStructureBuildGeometryInfoKHR build_info = vk_struct();
    {
        build_info.type = struct_type;
        build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.dstAccelerationStructure = blas;
        build_info.geometryCount = 1;
        build_info.pGeometries = &geometry;
        build_info.scratchData.deviceAddress = buffer_device_address(SubBuffer(scratch)).deviceAddress;
    }

    VkAccelerationStructureBuildRangeInfoKHR build_range = {};
    {
        build_range.primitiveCount = 1;
    }

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    {
        const VkAccelerationStructureBuildRangeInfoKHR* build_range_ptr = &build_range;
        vkCmdBuildAccelerationStructuresKHR(recorder.vk_cmd_buffer(), 1, &build_info, &build_range_ptr);
    }
    recorder.submit();

    return {std::move(blas), std::move(buffer)};
}



static std::pair<VkHandle<VkAccelerationStructureKHR>, Buffer<BufferUsage::AccelStructureBit>> create_blas(const MeshDrawData& mesh) {
    VkAccelerationStructureGeometryTrianglesDataKHR triangles = vk_struct();
    {
        const MeshDrawBuffers& mesh_buffers = mesh.mesh_buffers();
        const AttribSubBuffer& position_buffer = mesh_buffers.attrib_buffers()[usize(VertexStreamType::Position)];

        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexStride = sizeof(math::Vec3);
        triangles.vertexData = buffer_device_address(position_buffer);
        triangles.maxVertex = u32(mesh_buffers.vertex_count());

        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData = buffer_device_address(mesh_buffers.triangle_buffer());
    }

    return create_acceleration_structure(VkAccelerationStructureGeometryDataKHR { .triangles = triangles }, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
}





static std::pair<VkHandle<VkAccelerationStructureKHR>, Buffer<BufferUsage::AccelStructureBit>> create_tlas(core::Span<VkAccelerationStructureInstanceKHR> instances) {
    TypedBuffer<VkAccelerationStructureInstanceKHR, BufferUsage::AccelStructureInputBit, MemoryType::CpuVisible> instance_buffer(instances.size());
    {
        auto mapping = instance_buffer.map(MappingAccess::WriteOnly);
        std::copy(instances.begin(), instances.end(), mapping.begin());
    }

    VkAccelerationStructureGeometryInstancesDataKHR instances_data = vk_struct();
    {
        instances_data.data = buffer_device_address(SubBuffer(instance_buffer));
    }

    return create_acceleration_structure(VkAccelerationStructureGeometryDataKHR { .instances = instances_data }, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
}




AccelerationStructure::~AccelerationStructure() {
    destroy_graphic_resource(std::move(_acc_struct));
}

bool AccelerationStructure::is_null() const {
    return _acc_struct.is_null();
}

VkAccelerationStructureKHR AccelerationStructure::vk_acc_struct() const {
    return _acc_struct.get();
}

SubBuffer<BufferUsage::AccelStructureBit> AccelerationStructure::buffer() const {
    return _buffer;
}



BLAS::BLAS(const MeshDrawData& mesh) {
    std::tie(_acc_struct, _buffer) = create_blas(mesh);
}


TLAS::TLAS(core::Span<BLASInstance> instances) {
    std::tie(_acc_struct, _buffer) = create_tlas(instances);
}

BLASInstance TLAS::make_instance(const math::Transform<>& tr, const BLAS& blas) {
    BLASInstance instance = {};

    {
        instance.mask = 0xFF;

        VkAccelerationStructureDeviceAddressInfoKHR addr_info = vk_struct();
        addr_info.accelerationStructure = blas.vk_acc_struct();
        instance.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(vk_device(), &addr_info);

        const math::Matrix<4, 3> mat = tr.matrix().to<4, 3>();
        std::copy(mat.begin(), mat.end(), &instance.transform.matrix[0][0]);
    }

    return instance;
}

}

