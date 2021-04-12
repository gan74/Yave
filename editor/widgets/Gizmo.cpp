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
#include "Gizmo.h"

#include <editor/Selection.h>
#include <editor/EditorWorld.h>

#include <yave/scene/SceneView.h>
#include <yave/components/TransformableComponent.h>

#include <external/imgui/yave_imgui.h>


namespace editor {

static constexpr  u32 gizmo_hover_color = 0x001A80FF;

static constexpr float gizmo_hover_width = 7.5f;
static constexpr float gizmo_width = 2.5f;
static constexpr float gizmo_size = 0.25f;
static constexpr float gizmo_radius = 1.0f;
static constexpr float gizmo_pick_radius = 0.05f;
static constexpr u32 gizmo_alpha = 0xD0000000;

// stuff for the 2 axes selection
static constexpr float gizmo_quad_offset = 0.25f;
static constexpr float gizmo_quad_size = 0.3f;
static constexpr u32 gizmo_quad_alpha = 0xB0000000;


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




Gizmo::Gizmo(SceneView* view) : _scene_view(view) {
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

float Gizmo::snapping() const {
    return _snapping;
}

void Gizmo::set_snapping(float snapping) {
    _snapping = snapping;
}

float Gizmo::rotation_snapping() const {
    return _rot_snapping;
}

void Gizmo::set_rotation_snapping(float snapping) {
    _rot_snapping = snapping;
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
        (std::fabs(screen.x()) > std::fabs(screen.y()) ? screen.x() : screen.y()) = FP_INFINITE; // infs
    }

    return screen.to<2>() * viewport + offset;
}

float Gizmo::snap(float x) const {
    return _snapping == 0.0f
        ? x
        : std::round(x / _snapping) * _snapping;
}

float Gizmo::snap_rot(float x) const {
    return _rot_snapping == 0.0f
        ? x
        : std::round(x / _rot_snapping) * _rot_snapping;
}

void Gizmo::draw() {
    y_profile();

    if(!selection().has_selected_entity()) {
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

    TransformableComponent* transformable = current_world().component<TransformableComponent>(selection().selected_entity());
    if(!transformable) {
        return;
    }

    const auto axis_color = [](usize index) { return 0xFF << (8 * index); };

    math::Vec3 cam_fwd = _scene_view->camera().forward();
    math::Vec3 cam_pos = _scene_view->camera().position();
    math::Matrix4<> view_proj = _scene_view->camera().viewproj_matrix();
    auto [obj_pos, obj_rot, obj_scale] = transformable->transform().decompose();

    if(cam_fwd.dot(obj_pos - cam_pos) < 0.0f) {
        return;
    }

    const math::Vec4 projected = (view_proj * math::Vec4(obj_pos, 1.0f));
    const float perspective = gizmo_size * projected.w();

    const math::Vec2 mouse = math::Vec2(ImGui::GetIO().MousePos);
    const math::Vec2 viewport = ImGui::GetWindowSize();
    const math::Vec2 offset = ImGui::GetWindowPos();

    const auto inv_matrix = _scene_view->camera().inverse_matrix();
    const math::Vec2 ndc = ((mouse - offset) / viewport) * 2.0f - 1.0f;
    const math::Vec4 h_world = inv_matrix * math::Vec4(ndc, 0.5f, 1.0f);
    const math::Vec3 world = h_world.to<3>() / h_world.w();
    const math::Vec3 ray = (world - cam_pos).normalized();
    const float dist = (obj_pos - cam_pos).length();
    const math::Vec3 projected_mouse = cam_pos + ray * dist;

    const math::Vec2 center = to_window_pos(obj_pos);


    std::array<math::Vec3, 3> basis = {
        math::Vec3{1.0f, 0.0f, 0.0f},
        math::Vec3{0.0f, 1.0f, 0.0f},
        math::Vec3{0.0f, 0.0f, 1.0f}
    };


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
        std::sort(std::begin(axes), std::end(axes), [&](const Axis& a, const Axis& b) {
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

                const float min_len = gizmo_quad_offset * perspective;
                const float max_len = (gizmo_quad_size + gizmo_quad_offset) * perspective;
                if(da > min_len && db > min_len && da < max_len && db < max_len) {
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
                /*if(std::abs(basis[i].dot(cam_fwd)) < 0.25f) {
                    continue;
                }*/

                const usize a = (i + 1) % 3;
                const usize b = (i + 2) % 3;
                const u32 mask = axes[a].mask() | axes[b].mask();
                const bool hovered = (hover_mask & mask) == mask;
                const u32 color = hovered ? gizmo_hover_color : axis_color(axes[i].index);
                const math::Vec2 quad_offset = ((axes[a].vec - center)  + (axes[b].vec - center)) * gizmo_quad_offset;

                const auto smaller = [&] (const math::Vec2& v) { return (v - center) * gizmo_quad_size + center; };
                const ImVec2 pts[] = {
                        quad_offset + center,
                        quad_offset + smaller(axes[a].vec),
                        quad_offset + smaller(axes[a].vec + axes[b].vec - center),
                        quad_offset + smaller(axes[b].vec)
                    };
                ImGui::GetWindowDrawList()->AddConvexPolyFilled(pts, 4, gizmo_quad_alpha | color);
            }

            // axes
            for(usize k = 0; k != 3; ++k) {
                const usize i = 2 - k;
                const bool hovered = hover_mask & axes[i].mask();
                const u32 color = hovered ? gizmo_hover_color : axis_color(axes[i].index);
                ImGui::GetWindowDrawList()->AddLine(center, axes[i].vec, gizmo_alpha | color, gizmo_width);
            }
            ImGui::GetWindowDrawList()->AddCircleFilled(center, 1.5f * gizmo_width, 0xFFFFFFFF);
        }


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
            const math::Vec3 vec = new_pos - transformable->position();
            for(usize i = 0; i != 3; ++i) {
                if(_dragging_mask & (1 << i)) {
                    const math::Vec3 offset = basis[i] * vec.dot(basis[i]);
                    transformable->position() += math::Vec3(snap(offset.x()), snap(offset.y()), snap(offset.z()));
                }
            }
        }
    } else if(_mode == Rotate) {
        const usize segment_count = 64;
        const float inv_segment_count = 1.0f / segment_count;
        const float seg_ang_size = inv_segment_count * 2.0f * math::pi<float>;

        usize rotation_axis = _rotation_axis;

        auto cam_side = [&](math::Vec3 world_pos) {
            const math::Vec3 local = world_pos - obj_pos;
            return local.normalized().dot((world_pos - cam_pos).normalized()) < 0.1f;
        };


        auto next_point = [&](usize axis, usize i) -> std::pair<math::Vec2, bool> {
            const math::Vec3 radial = basis[axis] * std::sin(i * seg_ang_size) + basis[(axis + 1) % 3] * std::cos(i * seg_ang_size);
            return {to_window_pos(obj_pos + radial * gizmo_radius * perspective), cam_side(obj_pos + radial)};
        };

        // compute hover
        bool hovered = false;
        for(usize axis = 0; !hovered && axis != 3; ++axis) {
            const usize rot_axis = (axis + 2) % 3;

            math::Vec2 last_point = next_point(axis, 0).first;
            for(usize i = 1; i != segment_count + 1; ++i) {
                const auto [next, visible] = next_point(axis, i);
                y_defer(last_point = next);

                if(!visible) {
                    continue;
                }
                const math::Vec2 vec = next - last_point;
                if(vec.dot(next - mouse) < 0.0f || vec.dot(last_point - mouse) > 0.0f) {
                    continue;
                }
                const math::Vec2 ortho = math::Vec2(-vec.y(), vec.x()).normalized();
                if(std::abs(ortho.dot(next - mouse)) > 5.0f) {
                    continue;
                }
                rotation_axis = rot_axis;
                hovered = true;
                break;
            }
        }

        // draw
        const usize current_axis = (_rotation_axis == usize(-1) ? rotation_axis : _rotation_axis);
        for(usize axis = 0; axis != 3; ++axis) {
            const usize rot_axis = (axis + 2) % 3;
            const bool is_current_axis = rot_axis == current_axis;
            const u32 color = is_current_axis ? gizmo_hover_color :  axis_color(rot_axis);

            math::Vec2 last_point = next_point(axis, 0).first;
            for(usize i = 1; i != segment_count + 1; ++i) {
                const auto [next, visible] = next_point(axis, i);
                y_defer(last_point = next);

                u32 alpha = gizmo_alpha;
                if(!visible) {
                    if(!is_current_axis || i % 2) {
                        continue;
                    }
                    alpha /= 2;
                }
                ImGui::GetWindowDrawList()->AddLine(last_point, next, alpha | color, gizmo_width);
            }
        }

        auto compute_angle = [&](usize axis) {
                if(axis >= 3) {
                    return 0.0f;
                }
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
            const float angle_offset = snap_rot(angle - _rotation_offset);
            const math::Quaternion<> rot = math::Quaternion<>::from_axis_angle(basis[_rotation_axis], angle_offset);
            math::Transform<> tr = transformable->transform();
            tr.set_basis(rot(tr.forward()), rot(tr.right()), rot(tr.up()));
            transformable->set_transform(tr);
            _rotation_offset += angle_offset;
        }
    }
}


}

