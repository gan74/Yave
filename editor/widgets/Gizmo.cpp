/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include "Gizmo.h"

#include <editor/context/EditorContext.h>

#include <yave/components/TransformableComponent.h>

#include <imgui/yave_imgui.h>

#include <y/utils/sort.h>

namespace editor {

static constexpr float gizmo_hover_width = 7.5f;
static constexpr float gizmo_width = 2.5f;
static constexpr float gizmo_size = 0.15f;
static constexpr float gizmo_radius = 1.0f;
static constexpr float gizmo_pick_radius = 0.05f;
static constexpr u32 gizmo_alpha = 0xB0000000;

// stuff for the 2 axes selection
static constexpr float gizmo_size_mul_2 = 0.25f;
static constexpr u32 gizmo_alpha_2 = 0x70000000;


static bool is_clicked(bool allow_drag) {
    return allow_drag && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive();
}

static bool is_clicking(math::Vec2 cursor, const math::Vec2& vec) {
    float len = vec.length();
    const math::Vec2 n = vec / len;
    const float d = cursor.dot(n);

    if(d > len || d < 0.0f) {
        return false;
    }

    const float d_2 = std::fabs(cursor.length2() - (d * d));
    return d_2 < (gizmo_hover_width * gizmo_hover_width);
}

static math::Vec3 intersect(const math::Vec3& normal, const math::Vec3& center, const math::Vec3& start, const math::Vec3& end) {
    math::Vec3 direction = (end - start).normalized();
    const float denom = normal.dot(direction);
    const float t = (center - start).dot(normal) / denom;
    return start + direction * t;
}

static const ImU32 gizmo_flags =
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

Gizmo::Gizmo(ContextPtr cptr, SceneView* view) : ContextLinked(cptr), _scene_view(view) {
}

bool Gizmo::is_dragging() const {
    return _allow_drag && (_dragging_mask || _rotation_axis != usize(-1));
}

void Gizmo::set_allow_drag(bool allow) {
    _allow_drag = allow;
}


Gizmo::Mode Gizmo::mode() const {
    return _mode;
}

void Gizmo::set_mode(Mode mode) {
    _mode = mode;
}

Gizmo::Space Gizmo::space() const {
    return _space;
}

void Gizmo::set_space(Space space) {
    _space = space;
}

math::Vec3 Gizmo::to_screen_pos(const math::Vec3& world) {
    const auto h_pos = _scene_view->camera().viewproj_matrix() * math::Vec4(world, 1.0f);
    return math::Vec3((h_pos.to<2>() / h_pos.w()) * 0.5f + 0.5f, h_pos.z() / h_pos.w());
}

math::Vec2 Gizmo::to_window_pos(const math::Vec3& world) {
    math::Vec2 viewport = ImGui::GetWindowSize();
    math::Vec2 offset = ImGui::GetWindowPos();

    auto screen = to_screen_pos(world);

    if(screen.z() < 0.0f) {
        (std::fabs(screen.x()) > std::fabs(screen.y()) ? screen.x() : screen.y()) /= 0.0f; // infs
    }

    return screen.to<2>() * viewport + offset;
}

void Gizmo::draw() {
    if(!context()->selection().has_selected_entity()) {
        return;
    }

    y_defer(ImGui::EndChild());
    ImGui::SetNextWindowBgAlpha(0.0f);
    if(!ImGui::BeginChild("##gizmo", ImVec2(0, 0), false, gizmo_flags)) {
        return;
    }

    /*if(ImGui::IsKeyReleased(int(Key::R))) {
        _mode = Mode(!usize(_mode));
    }
    if(ImGui::IsKeyReleased(int(Key::A))) {
        _space = Space(!usize(_space));
    }*/

    TransformableComponent* transformable = context()->world().component<TransformableComponent>(context()->selection().selected_entity());
    if(!transformable) {
        return;
    }

    const auto axis_color = [](usize index) { return 0xFF << (8 * index); };
    const u32 hover_color = 0x001A80FF;

    math::Vec3 cam_fwd = _scene_view->camera().forward();
    math::Vec3 cam_pos = _scene_view->camera().position();
    math::Matrix4<> view_proj = _scene_view->camera().viewproj_matrix();
    auto [obj_pos, obj_rot, obj_scale] = transformable->transform().decompose();

    if(cam_fwd.dot(obj_pos - cam_pos) < 0.0f) {
        return;
    }

    const math::Vec4 projected = (view_proj * math::Vec4(obj_pos, 1.0f));
    const float perspective = gizmo_size * projected.w();

    const math::Vec2 viewport = ImGui::GetWindowSize();
    const math::Vec2 offset = ImGui::GetWindowPos();

    const auto inv_matrix = _scene_view->camera().inverse_matrix();
    const math::Vec2 ndc = ((math::Vec2(ImGui::GetIO().MousePos) - offset) / viewport) * 2.0f - 1.0f;
    const math::Vec4 h_world = inv_matrix * math::Vec4(ndc, 0.5f, 1.0f);
    const math::Vec3 world = h_world.to<3>() / h_world.w();
    const math::Vec3 ray = (world - cam_pos).normalized();
    const float dist = (obj_pos - cam_pos).length();
    const math::Vec3 projected_mouse = cam_pos + ray * dist;

    const auto center = to_window_pos(obj_pos);


    std::array<math::Vec3, 3> basis = {math::Vec3{1.0f, 0.0f, 0.0f},
                                       math::Vec3{0.0f, 1.0f, 0.0f},
                                       math::Vec3{0.0f, 0.0f, 1.0f}};


    {
        if(_space == Local) {
            for(math::Vec3& a : basis) {
                a = obj_rot(a);
            }
        }

        if(_mode == Translate) {
            for(math::Vec3& a : basis) {
                if(cam_fwd.dot(a) > 0.0f) {
                    a = -a;
                }
            }
        }
    }


    if(_mode == Translate) {
        struct Axis {
            math::Vec2 vec;
            usize index;
            u32 mask() const { return 1 << index; }
        };

        Axis axes[] = {
                {to_window_pos(obj_pos + basis[0] * perspective), 0},
                {to_window_pos(obj_pos + basis[1] * perspective), 1},
                {to_window_pos(obj_pos + basis[2] * perspective), 2}
            };

        // depth sort axes front to back
        sort(std::begin(axes), std::end(axes), [&](const Axis& a, const Axis& b) {
            return basis[a.index].dot(cam_fwd) < basis[b.index].dot(cam_fwd);
        });

        // compute hover
        u32 hover_mask = _dragging_mask;
        if(!hover_mask) {
            for(usize k = 0; k != 3; ++k) {
                const usize i = 2 - k;
                const usize index = axes[i].index;
                const math::Vec3 proj = intersect(basis[index], obj_pos, cam_pos, projected_mouse) - obj_pos;
                const float da = proj.dot(basis[(index + 1) % 3]);
                const float db = proj.dot(basis[(index + 2) % 3]);

                const float len = gizmo_size_mul_2 * perspective;
                if(da > 0.0f && db > 0.0f && da < len && db < len) {
                    hover_mask = axes[(i + 1) % 3].mask() | axes[(i + 2) % 3].mask();
                    break;
                }
            }

            // axes
            if(!hover_mask) {
                const math::Vec2 cursor = math::Vec2(ImGui::GetIO().MousePos) - center;
                for(usize i = 0; i != 3; ++i) {
                    if(is_clicking(cursor, axes[i].vec - center)) {
                        hover_mask = axes[i].mask();
                        break;
                    }
                }
            }
        }

        // draw
        {
            // quads
            for(usize i = 0; i != 3; ++i) {
                const usize a = (i + 1) % 3;
                const usize b = (i + 2) % 3;
                const u32 mask = axes[a].mask() | axes[b].mask();
                const bool hovered = (hover_mask & mask) == mask;
                const u32 color = hovered ? hover_color : axis_color(axes[i].index);

                const auto smaller = [&] (const math::Vec2& v) { return (v - center) * gizmo_size_mul_2 + center; };
                ImVec2 pts[] = {
                        center,
                        smaller(axes[a].vec),
                        smaller(axes[a].vec + axes[b].vec - center),
                        smaller(axes[b].vec)
                    };
                ImGui::GetWindowDrawList()->AddConvexPolyFilled(pts, 4, gizmo_alpha_2 | color);
            }

            // axes
            for(usize k = 0; k != 3; ++k) {
                const usize i = 2 - k;
                const bool hovered = hover_mask & axes[i].mask();
                const u32 color = hovered ? hover_color : axis_color(axes[i].index);
                ImGui::GetWindowDrawList()->AddLine(center, axes[i].vec, gizmo_alpha | color, gizmo_width);
            }
            ImGui::GetWindowDrawList()->AddCircleFilled(center, 1.5f * gizmo_width, 0xFFFFFFFF);
        }

        /*math::Vec3 plan_normal(1.0f);
        for(usize i = 0; i != 3; ++i) {
            u32 mask = 1 << i;
            if(_dragging_mask & mask) {
                plan_normal[i] = 0.0f;
            }
        }
        projected_mouse = intersect(plan_normal, obj_pos, cam_pos, world);*/

        // click
        {
            if(is_clicked(_allow_drag)) {
                _dragging_mask = hover_mask;
                _dragging_offset = obj_pos - projected_mouse;
            } else if(!ImGui::IsMouseDown(0)) {
                _dragging_mask = 0;
            }
        }

        // drag
        if(_dragging_mask) {
            const math::Vec3 new_pos = projected_mouse + _dragging_offset;
            math::Vec3 vec = new_pos - transformable->position();
            for(usize i = 0; i != 3; ++i) {
                if(_dragging_mask & (1 << i)) {
                    transformable->position() += basis[i] * vec.dot(basis[i]);
                }
            }
        }
    } else if(_mode == Rotate) {
        const usize segment_count = 64;
        const float seg_ang_size = (1.0f / segment_count) * 2.0f * math::pi<float>;
        usize rotation_axis = _rotation_axis;

        for(usize axis = 0; axis != 3; ++axis) {
            const usize rot_axis = (axis + 2) % 3;
            const math::Vec3 proj = intersect(basis[rot_axis], obj_pos, cam_pos, projected_mouse);
            const bool hovered = std::abs((obj_pos - proj).length() - perspective) < gizmo_pick_radius * perspective;
            if(hovered) {
                rotation_axis = rot_axis;
            }

            const u32 color = rotation_axis == rot_axis ? hover_color :  axis_color(rot_axis);

            auto screen_pos = [&](usize i) {
                    const math::Vec3 pos = basis[axis] * std::sin(i * seg_ang_size) + basis[(axis + 1) % 3] * std::cos(i * seg_ang_size);
                    return to_window_pos(obj_pos + pos * gizmo_radius * perspective);
                };

            math::Vec2 last = screen_pos(0);
            for(usize i = 1; i != segment_count + 1; ++i) {
                const math::Vec2 next = screen_pos(i);
                ImGui::GetWindowDrawList()->AddLine(last, next, gizmo_alpha | color, gizmo_width);
                last = next;
            }
        }

        auto compute_angle = [&](usize axis) {
                const math::Vec3 proj = intersect(basis[axis], obj_pos, cam_pos, projected_mouse);
                const math::Vec3 vec = (proj - obj_pos).normalized();
                return std::copysign(std::acos(vec[(axis + 1) % 3]), vec[(axis + 2) % 3]);
            };

        if(is_clicked(_allow_drag)) {
            _rotation_axis = rotation_axis;
            _rotation_offset = compute_angle(_rotation_axis);
        } if(!ImGui::IsMouseDown(0)) {
            _rotation_axis = usize(-1);
        }

        if(_rotation_axis != usize(-1)) {
            const float angle = compute_angle(_rotation_axis);
            const math::Quaternion<> rot = math::Quaternion<>::from_axis_angle(basis[_rotation_axis], (angle - _rotation_offset));
            math::Transform<>& tr = transformable->transform();
            tr.set_basis(rot(tr.forward()), rot(tr.left()), rot(tr.up()));
            _rotation_offset = angle;
        }
    }
}


}

