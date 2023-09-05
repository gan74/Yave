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

#include "DirectDraw.h"

#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/utils/color.h>

namespace yave {


DirectDrawPrimitive::DirectDrawPrimitive(std::string_view name) : _name(name) {
}

void DirectDrawPrimitive::set_color(const math::Vec3& color) {
    set_color(pack_to_u32(math::Vec4(color, 1.0f)));
}

void DirectDrawPrimitive::set_color(u32 color) {
    _color = color;
}

u32 DirectDrawPrimitive::color() const {
    return _color;
}

void DirectDrawPrimitive::push_vertex(const math::Vec3& v) {
    _vertices << DirectVertex{v, _color};
}

void DirectDrawPrimitive::add_line(const math::Vec3 &a, const math::Vec3 &b) {
    push_vertex(a);
    push_vertex(b);
}

void DirectDrawPrimitive::add_circle(const math::Vec3& position, math::Vec3 x, math::Vec3 y, float radius, usize divs) {
    x *= radius;
    y *= radius;
    const float seg_ang_size = (1.0f / divs) * 2.0f * math::pi<float>;

    math::Vec3 last = position + y;
    for(usize i = 1; i != divs + 1; ++i) {
        const math::Vec2 c(std::sin(i * seg_ang_size), std::cos(i * seg_ang_size));
        push_vertex(last);
        last = (position + (x * c.x()) + (y * c.y()));
        push_vertex(last);
    }
}

void DirectDrawPrimitive::add_cone(const math::Vec3& position, math::Vec3 x, math::Vec3 y, float len, float angle, usize divs, usize circle_subdivs) {
    const math::Vec3 z = x.cross(y).normalized();

    const usize beg = _vertices.size();
    add_circle(position + x * (std::cos(angle) * len), y, z, std::sin(angle) * len, circle_subdivs * divs);

    for(usize i = 0; i != divs; ++i) {
        push_vertex(_vertices[beg + (i * 2) * circle_subdivs].pos);
        push_vertex(position);
    }
}

void DirectDrawPrimitive::add_box(const AABB& aabb, const math::Transform<>& transform) {
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
            push_vertex(transform.transform_point(center + a));
            push_vertex(transform.transform_point(center + b));
        }
    }
}

void DirectDrawPrimitive::add_marker(const math::Vec3& position, float size) {
    for(usize i = 0; i != 3; ++i) {
        math::Vec3 dir;
        dir[i] = size;
        add_line(position + dir, position - dir);
    }
}

core::Span<DirectVertex> DirectDrawPrimitive::vertices() const {
    return _vertices;
}




void DirectDraw::clear() {
    _primitives.clear();
}

DirectDrawPrimitive* DirectDraw::add_primitive(std::string_view name, const math::Vec3& color) {
    auto& prim = _primitives.emplace_back(std::make_unique<DirectDrawPrimitive>(name));
    prim->set_color(color);

    return prim.get();
}

core::Span<std::unique_ptr<DirectDrawPrimitive>> DirectDraw::primtitives() const {
    return _primitives;
}

void DirectDraw::render(RenderPassRecorder& recorder, const math::Matrix4<>& view_proj) const {
    y_profile();

    usize vertex_count = 0;
    for(const auto& prim : _primitives) {
        vertex_count += prim->_vertices.size();
    }

    if(!vertex_count) {
        return;
    }

    TypedAttribBuffer<DirectVertex, MemoryType::CpuVisible> vertices(vertex_count);
    auto mapping = vertices.map(MappingAccess::WriteOnly);

    {
        usize offset = 0;
        for(const auto& prim : _primitives) {
            const core::Span<DirectVertex> verts = prim->vertices();
            std::copy(verts.begin(), verts.end(), mapping.data() + offset);
            offset += verts.size();
        }

    }

    const auto* material = device_resources()[DeviceResources::WireFrameMaterialTemplate];
    recorder.bind_material_template(material, DescriptorSet(InlineDescriptor(view_proj)));
    recorder.bind_attrib_buffers({vertices});
    recorder.draw_array(vertex_count);
}

}

