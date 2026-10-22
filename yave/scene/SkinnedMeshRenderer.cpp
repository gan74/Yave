/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "SkinnedMeshRenderer.h"
#include "Scene.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

namespace yave {

void SkinnedMeshRenderer::update(ComputeCapableCmdBufferRecorder& recorder, core::MutableSpan<SkinnedMeshObject> objects) {
    y_profile();

    if(objects.is_empty()) {
        _mesh_datas = {};
        return;
    }

    const auto region = recorder.region("Skinning");
    const auto& program = device_resources()[DeviceResources::SkinningProgram];

    MeshDataBuffer mesh_datas(objects.size());
    {
        auto mapping = mesh_datas.map(MappingAccess::WriteOnly);

        for(usize i = 0; i != objects.size(); ++i) {
            SkinnedMeshObject& obj = objects[i];
            mapping[i] = {};

            const StaticMesh* mesh = obj.component.mesh().get();
            SkeletonInstance* instance = obj.component.skeleton_instance();
            const u32 vertex_count = mesh ? mesh->draw_data().vertex_count() : 0;
            if(!instance || !vertex_count) {
                obj.skinned_positions = {};
                continue;
            }

            const u64 byte_size = vertex_count * sizeof(math::Vec3);
            if(obj.skinned_positions.byte_size() != byte_size) {
                obj.skinned_positions = PositionBuffer(byte_size);
            }

            instance->update();

            const MeshDrawBuffers buffers = mesh->draw_data().mesh_buffers();

            const auto descriptors = make_descriptor_set(
                obj.skinned_positions,
                SubBuffer<BufferUsage::StorageBit>(buffers.attribs[usize(VertexStreamType::Position)]),
                mesh->skin_weights(),
                instance->bone_transform_buffer(),
                InlineDescriptor(vertex_count)
            );

            recorder.dispatch_threads(program, math::Vec2ui(vertex_count, 1), DescriptorSetProxy(descriptors));

            mapping[i] = shader::StaticMeshData {
                obj.skinned_positions.vk_device_address(),
                buffers.attribs[usize(VertexStreamType::NormalTangent)].vk_device_address(),
                buffers.attribs[usize(VertexStreamType::Uv)].vk_device_address(),
                0
            };
        }
    }

    _mesh_datas = std::move(mesh_datas);
}

bool SkinnedMeshRenderer::is_ready(const SkinnedMeshObject& object) {
    return !object.skinned_positions.is_null();
}

SubBuffer<BufferUsage::StorageBit> SkinnedMeshRenderer::mesh_data_buffer() const {
    return _mesh_datas;
}

}

