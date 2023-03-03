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
#include "StaticMesh.h"
#include "MeshData.h"

#include <yave/graphics/device/MeshAllocator.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/commands/CmdQueue.h>

namespace yave {

StaticMesh::StaticMesh(const MeshData& mesh_data) :
    _draw_data(mesh_allocator().alloc_mesh(mesh_data.vertices(), mesh_data.triangles())),
    _aabb(mesh_data.aabb())  {

    const auto sub_meshes = mesh_data.sub_meshes();
    _sub_meshes = core::FixedArray<MeshDrawCommand>(sub_meshes.size());
    std::transform(sub_meshes.begin(), sub_meshes.end(), _sub_meshes.begin(), [cmd = _draw_data.draw_command()](auto sub_mesh) {
        return MeshDrawCommand {
            sub_mesh.triangle_count * 3,
            sub_mesh.first_triangle * 3 + cmd.first_index,
            cmd.vertex_offset
        };
    });

    {
        auto [area, surfs] = mesh_data.generate_surfels();

        _surfels = std::move(surfs);
        _total_area = area;

        _surfel_buffer = TypedBuffer<Surfel, BufferUsage::StorageBit | BufferUsage::TransferDstBit>(_surfels.size());

        CmdBufferRecorder recorder = create_disposable_cmd_buffer();
        BufferMappingBase::stage(_surfel_buffer, recorder, _surfels.data());
        loading_command_queue().submit(std::move(recorder));
    }
}

StaticMesh::~StaticMesh() {
    destroy_graphic_resource(std::move(_draw_data));
}

bool StaticMesh::is_null() const {
    return _draw_data.is_null();
}

const MeshDrawData& StaticMesh::draw_data() const {
    return _draw_data;
}

const MeshDrawCommand& StaticMesh::draw_command() const {
    return _draw_data.draw_command();
}

const core::Span<MeshDrawCommand> StaticMesh::sub_meshes() const {
    return _sub_meshes;
}

float StaticMesh::radius() const {
    return _aabb.origin_radius();
}

const AABB& StaticMesh::aabb() const {
    return _aabb;
}


}

