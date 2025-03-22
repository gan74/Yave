/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "DirectDraw.h"

#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/utils/color.h>

namespace yave {

DirectDrawPrimitive::DirectDrawPrimitive(std::string_view name) : _name(name) {
}

void DirectDrawPrimitive::push_wire_vertex(const math::Vec3& v, u32 color) {
    _wire_vertices << DirectVertex{v, color};
}

void DirectDrawPrimitive::add_line(u32 color, const math::Vec3 &a, const math::Vec3 &b) {
    push_wire_vertex(a, color);
    push_wire_vertex(b, color);
}

void DirectDrawPrimitive::add_triangle(u32 color, const math::Vec3& a, const math::Vec3& b, const math::Vec3& c) {
    _triangle_vertices << DirectVertex{a, color};
    _triangle_vertices << DirectVertex{b, color};
    _triangle_vertices << DirectVertex{c, color};
}

void DirectDrawPrimitive::add_circle(u32 color, const math::Vec3& position, math::Vec3 x, math::Vec3 y, float radius, usize divs) {
    x *= radius;
    y *= radius;
    const float seg_ang_size = (1.0f / divs) * 2.0f * math::pi<float>;

    math::Vec3 last = position + y;
    for(usize i = 1; i != divs + 1; ++i) {
        const math::Vec2 c(std::sin(i * seg_ang_size), std::cos(i * seg_ang_size));
        push_wire_vertex(last, color);
        last = (position + (x * c.x()) + (y * c.y()));
        push_wire_vertex(last, color);
    }
}

void DirectDrawPrimitive::add_wire_cone(u32 color, const math::Vec3& position, math::Vec3 x, math::Vec3 y, float len, float angle, usize divs, usize circle_subdivs) {
    const math::Vec3 z = x.cross(y).normalized();

    const usize beg = _wire_vertices.size();
    add_circle(color, position + x * (std::cos(angle) * len), y, z, std::sin(angle) * len, circle_subdivs * divs);

    for(usize i = 0; i != divs; ++i) {
        push_wire_vertex(_wire_vertices[beg + (i * 2) * circle_subdivs].pos, color);
        push_wire_vertex(position, color);
    }
}

void DirectDrawPrimitive::add_wire_box(u32 color, const AABB& aabb, const math::Transform<>& transform) {
    const math::Vec3 size = aabb.half_extent();
    const std::array corners = {
        size,
        math::Vec3(-size.x(), -size.y(), size.z()),
        math::Vec3(size.x(), -size.y(), -size.z()),
        math::Vec3(-size.x(), size.y(), -size.z()),
    };
    const math::Vec3 center = aabb.center();
    for(const math::Vec3 a : corners) {
        for(usize i = 0; i != 3; ++i) {
            math::Vec3 b = a;
            b[i] *= -1.0f;
            push_wire_vertex(transform.transform_point(center + a), color);
            push_wire_vertex(transform.transform_point(center + b), color);
        }
    }
}

void DirectDrawPrimitive::add_sphere_3circles(u32 color, const math::Vec3& position, float radius, usize divs) {
    add_circle(color, position, math::Vec3(1.0f, 0.0f, 0.0f), math::Vec3(0.0f, 1.0f, 0.0f), radius, divs);
    add_circle(color, position, math::Vec3(0.0f, 1.0f, 0.0f), math::Vec3(0.0f, 0.0f, 1.0f), radius, divs);
    add_circle(color, position, math::Vec3(0.0f, 0.0f, 1.0f), math::Vec3(1.0f, 0.0f, 0.0f), radius, divs);
}

void DirectDrawPrimitive::add_marker(u32 color, const math::Vec3& position, float size) {
    for(usize i = 0; i != 3; ++i) {
        math::Vec3 dir;
        dir[i] = size;
        add_line(color, position + dir, position - dir);
    }
}







void DirectDraw::clear() {
    auto lock = std::unique_lock(_lock);
    _primitives.clear();
}

DirectDrawPrimitive* DirectDraw::add_primitive(std::string_view name) {
    auto lock = std::unique_lock(_lock);
    return _primitives.emplace_back(std::make_unique<DirectDrawPrimitive>(name)).get();
}

void DirectDraw::render(RenderPassRecorder& recorder, const math::Matrix4<>& view_proj) const {
    y_profile();

    auto lock = std::unique_lock(_lock);

    usize wire_vertex_count = 0;
    usize triangle_vertex_count = 0;
    for(const auto& prim : _primitives) {
        wire_vertex_count += prim->_wire_vertices.size();
        triangle_vertex_count += prim->_triangle_vertices.size();
    }

    const usize vertex_count = wire_vertex_count + triangle_vertex_count;
    if(!vertex_count) {
        return;
    }

    TypedAttribBuffer<DirectVertex, MemoryType::CpuVisible> vertices(vertex_count);
    {
        auto mapping = vertices.map(MappingAccess::WriteOnly);

        usize offset = 0;
        for(const auto& prim : _primitives) {
            std::copy(prim->_wire_vertices.begin(), prim->_wire_vertices.end(), mapping.data() + offset);
            offset += prim->_wire_vertices.size();
        }
        y_debug_assert(offset == wire_vertex_count);

        for(const auto& prim : _primitives) {
            std::copy(prim->_triangle_vertices.begin(), prim->_triangle_vertices.end(), mapping.data() + offset);
            offset += prim->_triangle_vertices.size();
        }
        y_debug_assert(offset == vertex_count);
    }

    struct Params {
        math::Matrix4<> view_proj;
        math::Vec3 padding;
        u32 lit;
    };

    recorder.bind_attrib_buffers({vertices});
    {
        const auto descriptors = make_descriptor_set(InlineDescriptor(Params{view_proj, {}, 0}));
        recorder.bind_material_template(device_resources()[DeviceResources::DirectDrawWireMaterialTemplate], DescriptorSetProxy(descriptors));
        recorder.draw_array(wire_vertex_count, 1, 0);
    }
    {
        const auto descriptors = make_descriptor_set(InlineDescriptor(Params{view_proj, {}, 1}));
        recorder.bind_material_template(device_resources()[DeviceResources::DirectDrawMaterialTemplate], DescriptorSetProxy(descriptors));
        recorder.draw_array(triangle_vertex_count, 1, wire_vertex_count);
    }
}

}

