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

#include "DirectDraw.h"

#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/utils/color.h>

namespace yave {

void DirectDrawPrimitive::add_circle(const math::Vec3& position, math::Vec3 x, math::Vec3 y, float radius, usize divs) {
    x *= radius;
    y *= radius;
    const float seg_ang_size = (1.0f / divs) * 2.0f * math::pi<float>;

    math::Vec3 last = position + y;
    for(usize i = 1; i != divs + 1; ++i) {
        const math::Vec2 c(std::sin(i * seg_ang_size), std::cos(i * seg_ang_size));
        _points << last;
        last = (position + (x * c.x()) + (y * c.y()));
        _points << last;
    }
}

void DirectDrawPrimitive::add_cone(const math::Vec3& position, math::Vec3 x, math::Vec3 y, float len, float angle, usize divs, usize circle_subdivs) {
    const math::Vec3 z = x.cross(y).normalized();

    const usize beg = _points.size();
    add_circle(position + x * (std::cos(angle) * len), y, z, std::sin(angle) * len, circle_subdivs * divs);

    for(usize i = 0; i != divs; ++i) {
        _points << _points[beg + (i * 2) * circle_subdivs];
        _points << position;
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
            _points << transform.to_global(center + a) << transform.to_global(center + b);
        }
    }
}

core::Span<math::Vec3> DirectDrawPrimitive::points() const {
    return _points;
}



DirectDrawPrimitive* DirectDraw::add_primitive(const math::Vec3& color) {
    _primitives.emplace_back(std::make_unique<DirectDrawPrimitive>());

    DirectDrawPrimitive* prim = _primitives.last().get();
    prim->_color = pack_to_u32(math::Vec4(color, 1.0f));

    return prim;
}

core::Span<std::unique_ptr<DirectDrawPrimitive>> DirectDraw::primtitives() const {
    return _primitives;
}

void DirectDraw::render(RenderPassRecorder& recorder, const math::Matrix4<>& view_proj) const {
    y_profile();

    usize point_count = 0;
    for(const auto& prim : _primitives) {
        point_count += prim->_points.size();
    }

    if(!point_count) {
        return;
    }

    struct DirectVertex {
        math::Vec3 pos;
        u32 color;
    };

    TypedAttribBuffer<DirectVertex, MemoryType::CpuVisible> vertices(point_count);
    TypedMapping mapping(vertices);

    {
        usize offset = 0;
        for(const auto& prim : _primitives) {
            for(const math::Vec3& p : prim->_points) {
                mapping[offset++] = {
                    p,
                    prim->_color
                };
            }
        }

    }

    const auto* material = device_resources()[DeviceResources::WireFrameMaterialTemplate];
    recorder.bind_material(material, {DescriptorSet({InlineDescriptor(view_proj)})});
    recorder.bind_attrib_buffers(vertices);
    recorder.draw_array(point_count);
}

}

