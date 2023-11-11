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
#include "Gizmo.h"

#include <editor/UndoStack.h>
#include <editor/EditorWorld.h>

#include <yave/scene/SceneView.h>
#include <yave/components/TransformableComponent.h>

#include <editor/utils/ui.h>

#include <yave/utils/DirectDraw.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

static constexpr float gizmo_hover_width = 7.5f;
static constexpr float gizmo_width = 2.5f;
static constexpr float gizmo_size = 0.25f;
static constexpr float gizmo_radius = 1.0f;
static constexpr u32 gizmo_alpha = 0xD0000000;

// stuff for the 2 axes selection
static constexpr float gizmo_quad_offset = 0.25f;
static constexpr float gizmo_quad_size = 0.3f;
static constexpr u32 gizmo_quad_alpha = 0xB0000000;

static const u32 gizmo_hover_color = pack_to_u32(sRGB_to_linear(unpack_from_u32(0x001A80FF)));



static constexpr float orientation_gizmo_size = 25.0f;
static constexpr float orientation_gizmo_width = 2.5f;
static constexpr float orientation_gizmo_end_point_width = 8.0f;

static constexpr u32 orientation_gizmo_alpha = 0xB0000000;





static bool is_clicked() {
    return ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive();
}

static bool is_hovering_axis(math::Vec2 a, math::Vec2 b) {
    const math::Vec2 mouse = math::Vec2(ImGui::GetIO().MousePos);
    const math::Vec2 to_mouse = mouse - b;
    const math::Vec2 vec = a - b;


    float len = vec.length();
    const math::Vec2 n = vec / len;
    const float d = to_mouse.dot(n);

    if(d > len || d < 0.0f) {
        return false;
    }

    const float d_2 = std::fabs(to_mouse.length2() - (d * d));
    return d_2 < (gizmo_hover_width * gizmo_hover_width);
}

static math::Vec3 intersect_ray_plane(const math::Vec3& orig, const math::Vec3& dir, const math::Vec3& normal, float d) {
    const float denom = normal.dot(dir);
    const float t = -(normal.dot(orig) + d) / denom;
    return orig + dir * t;
}

static [[nodiscard]] bool intersect_lines(const math::Vec3& start_a, const math::Vec3& end_a, const math::Vec3& start_b, const math::Vec3& end_b, math::Vec3& closest) {
    const math::Vec3 starts = start_a - start_b;
    const math::Vec3 ends = end_b - start_b;
    const math::Vec3 dir_a = end_a - start_a;

    const float ends_sq_len = ends.length2();
    const float dir_sq_len = dir_a.length2();

    if(ends_sq_len < math::epsilon<float> || dir_sq_len < math::epsilon<float>) {
        return false;
    }

    const float ends_dot_a = ends.dot(dir_a);
    const float denom = dir_sq_len * ends_sq_len - ends_dot_a * ends_dot_a;

    if (std::abs(denom) < math::epsilon<float>) {
        return false;
    }

    const float num = starts.dot(ends) * ends_dot_a - starts.dot(dir_a) * ends_sq_len;
    closest = start_a + dir_a * (num / denom);

    return true;
}

static math::Vec2 closest_to_line(const math::Vec2& start, const math::Vec2& end, const math::Vec2& p) {
    const math::Vec2 dir = (end - start).normalized();
    const math::Vec2 to_p = p - start;
    return start + dir * dir.dot(to_p);
}

static bool is_inside(core::Span<math::Vec2> pts, const math::Vec2& p) {
    if(pts.size() < 3) {
        return false;
    }

    usize pos = 0;
    usize neg = 0;
    for(usize i = 0; i != pts.size(); ++i) {
        const math::Vec2 a = pts[i];
        const math::Vec2 b = pts[(i + 1) % pts.size()];
        const float d = (p.x() - a.x()) * (b.y() - a.y()) - (p.y() - a.y()) * (b.x() - a.x());

        if(d < 0.0f) {
            ++neg;
        }
        if(d > 0.0f) {
            ++pos;
        }

        if(pos && neg) {
            return false;
        }
    }
    return true;
}

static void move_recursive(EditorWorld& world, ecs::EntityId id, math::Transform<> old_parent_transform, math::Transform<> new_parent_transform) {
    if(TransformableComponent* component = world.component_mut<TransformableComponent>(id)) {
        const math::Transform<> tr = component->transform();
        component->set_transform(new_parent_transform * old_parent_transform.inverse() * tr);
        old_parent_transform = tr;
        new_parent_transform = component->transform();
    }

    for(const ecs::EntityId child : world.children(id)) {
        move_recursive(world, child, old_parent_transform, new_parent_transform);
    }
}





GizmoBase::GizmoBase(SceneView* view) : _scene_view(view) {
}

GizmoBase::~GizmoBase() {
}

void GizmoBase::set_allow_dragging(bool allow) {
    _allow_dragging = allow;
}

bool GizmoBase::is_dragging() const {
    return _is_dragging;
}

math::Vec3 GizmoBase::to_screen_pos(const math::Vec3& world) const {
    const math::Vec4 h_pos = _scene_view->camera().view_proj_matrix() * math::Vec4(world, 1.0f);
    return math::Vec3((h_pos.to<2>() / h_pos.w()) * 0.5f + 0.5f, h_pos.z() / h_pos.w());
}

math::Vec2 GizmoBase::to_window_pos(const math::Vec3& world) const {
    const math::Vec2 viewport = ImGui::GetWindowSize();
    const math::Vec2 offset = ImGui::GetWindowPos();

    auto screen = to_screen_pos(world);

    if(screen.z() < 0.0f) {
        (std::fabs(screen.x()) > std::fabs(screen.y()) ? screen.x() : screen.y()) = FP_INFINITE; // infs
    }

    return screen.to<2>() * viewport + offset;
}

math::Vec3 GizmoBase::to_world_pos(const math::Vec2& window) const {
    const math::Vec2 viewport = ImGui::GetWindowSize();
    const math::Vec2 offset = ImGui::GetWindowPos();

    const math::Vec2 ndc = ((window - offset) / viewport) * 2.0f - 1.0f;
    const math::Vec4 h_pos = _scene_view->camera().inverse_matrix() * math::Vec4(ndc, 0.5f, 1.0f);
    return h_pos.to<3>() / h_pos.w();
}





TranslationGizmo::TranslationGizmo(SceneView* view) : GizmoBase(view) {
}

bool TranslationGizmo::is_hovered(usize axis) const {
    return (_hover_mask & (1 << axis)) != 0;
}

void TranslationGizmo::draw() {
    _is_dragging &= ImGui::IsMouseDown(ImGuiMouseButton_Left);

    EditorWorld& world = current_world();
    const ecs::EntityId selected = world.selected_entity();

    if(!selected.is_valid()) {
        return;
    }

    const TransformableComponent* transformable = world.component<TransformableComponent>(selected);

    if(!transformable) {
        return;
    }




    const math::Vec2 mouse_pos = math::Vec2(ImGui::GetIO().MousePos);

    const math::Vec3 cam_pos = _scene_view->camera().position();
    const math::Vec3 cam_fwd = _scene_view->camera().forward();
    const math::Matrix4<> view_proj = _scene_view->camera().view_proj_matrix();

    const auto [obj_pos, obj_rot, obj_scale] = transformable->transform().decompose();

    if(cam_fwd.dot(obj_pos - cam_pos) < 0.0f || (cam_pos - obj_pos).length() < math::epsilon<float>) {
        return;
    }

    const float perspective = (view_proj * math::Vec4(obj_pos, 1.0f)).w() * gizmo_size;
    const math::Vec2 gizmo_screen_center = to_window_pos(obj_pos);


    const std::array basis = {
        obj_rot(math::Vec3(1.0f, 0.0f, 0.0f)).normalized(),
        obj_rot(math::Vec3(0.0f, 1.0f, 0.0f)).normalized(),
        obj_rot(math::Vec3(0.0f, 0.0f, 1.0f)).normalized()
    };

    const std::array end_point_factors = {
        perspective, perspective, perspective
    };

    const std::array end_points = {
        obj_pos + basis[0] * end_point_factors[0],
        obj_pos + basis[1] * end_point_factors[1],
        obj_pos + basis[2] * end_point_factors[2]
    };

    const std::array screen_end_points = {
        to_window_pos(end_points[0]),
        to_window_pos(end_points[1]),
        to_window_pos(end_points[2])
    };

    std::array indices = {
        0_uu, 1_uu, 2_uu
    };

    std::sort(indices.begin(), indices.end(), [&](usize a, usize b) {
        return (cam_pos - end_points[a]).length2() > (cam_pos - end_points[b]).length2();
    });

    const std::array rev_indices = {
        indices[2], indices[1], indices[0]
    };





    auto build_quad = [&](usize i) {
        const u32 x = (i + 1) % 3;
        const u32 y = (i + 2) % 3;
        const auto smaller = [&] (const math::Vec2& v) { return (v - gizmo_screen_center) * gizmo_quad_size + gizmo_screen_center; };
        const math::Vec2 offset = ((screen_end_points[x] - gizmo_screen_center) + (screen_end_points[y] - gizmo_screen_center)) * gizmo_quad_offset;
        const std::array pts = {
            offset + gizmo_screen_center,
            offset + smaller(screen_end_points[x]),
            offset + smaller(screen_end_points[x] + screen_end_points[y] - gizmo_screen_center),
            offset + smaller(screen_end_points[y])
        };
        return pts;
    };








    auto set_position = [&](const math::Vec3& pos) {
        const math::Transform<> old_transform = transformable->transform();
        math::Transform<> new_transform = old_transform;
        new_transform.position() = pos;

        move_recursive(world, selected, old_transform, new_transform);
    };



    if(_is_dragging && _allow_dragging) {
        const math::Vec2 target_screen_pos = mouse_pos - _dragging_offset;

        math::Vec3 target_obj_pos = obj_pos;
        for(usize i = 0; i != 3; ++i) {
            if(is_hovered(i)) {
                const math::Vec2 target_axis_screen_pos = closest_to_line(gizmo_screen_center, screen_end_points[i], target_screen_pos);
                math::Vec3 pos;
                if(intersect_lines(obj_pos, end_points[i], cam_pos, to_world_pos(target_axis_screen_pos), pos)) {
                    const math::Vec3 axis = basis[i];
                    target_obj_pos += (pos.dot(basis[i]) * axis) - (target_obj_pos.dot(axis) * axis);
                }
            }
        }


        set_position(target_obj_pos);
    } else {
        _hover_mask = 0;

        for(usize i : indices) {
            if(is_inside(build_quad(i), mouse_pos)) {
                _hover_mask |= (0x07 & ~(1 << i));
                break;
            }
        }

        for(usize i : rev_indices) {
            if(is_hovering_axis(gizmo_screen_center, screen_end_points[i])) {
                _hover_mask |= (1 << i);
                break;
            }
        }

        _is_dragging = is_clicked() && _hover_mask;
        _dragging_offset = mouse_pos - gizmo_screen_center;
    }




    // Quads
    for(usize i : rev_indices) {
        const std::array quad = build_quad(i);
        const ImVec2 pts[] = {
            quad[0], quad[1], quad[2], quad[3]
        };

        const u32 color = (is_hovered((i + 1) % 3) && is_hovered((i + 2) % 3)) ? gizmo_hover_color : imgui::gizmo_color(i);
        ImGui::GetWindowDrawList()->AddConvexPolyFilled(pts, 4, gizmo_quad_alpha | color);
    }

    // Axis
    for(usize i : indices) {
        const u32 color = is_hovered(i) ? gizmo_hover_color : imgui::gizmo_color(i);
        ImGui::GetWindowDrawList()->AddLine(gizmo_screen_center, screen_end_points[i], gizmo_alpha | color, gizmo_width);
    }


    ImGui::GetWindowDrawList()->AddCircleFilled(gizmo_screen_center, 1.5f * gizmo_width, 0xFFFFFFFF);
}

























RotationGizmo::RotationGizmo(SceneView* view) : GizmoBase(view) {
}

void RotationGizmo::draw() {
    _is_dragging &= ImGui::IsMouseDown(ImGuiMouseButton_Left);

    EditorWorld& world = current_world();
    const ecs::EntityId selected = world.selected_entity();

    if(!selected.is_valid()) {
        return;
    }

    const TransformableComponent* transformable = world.component<TransformableComponent>(selected);

    if(!transformable) {
        return;
    }




    const math::Vec2 mouse_pos = math::Vec2(ImGui::GetIO().MousePos);

    const math::Vec3 cam_pos = _scene_view->camera().position();
    const math::Vec3 cam_fwd = _scene_view->camera().forward();
    const math::Matrix4<> view_proj = _scene_view->camera().view_proj_matrix();

    const auto [obj_pos, obj_rot, obj_scale] = transformable->transform().decompose();

    if(cam_fwd.dot(obj_pos - cam_pos) < 0.0f || (cam_pos - obj_pos).length() < math::epsilon<float>) {
        return;
    }

    const float perspective = (view_proj * math::Vec4(obj_pos, 1.0f)).w() * gizmo_size;

    const std::array basis = {
        math::Vec3(1.0f, 0.0f, 0.0f),
        math::Vec3(0.0f, 1.0f, 0.0f),
        math::Vec3(0.0f, 0.0f, 1.0f)
    };

    const std::array end_point_factors = {
        perspective, perspective, perspective
    };

    const std::array end_points = {
        obj_pos + basis[0] * end_point_factors[0],
        obj_pos + basis[1] * end_point_factors[1],
        obj_pos + basis[2] * end_point_factors[2]
    };

    std::array indices = {
        0_uu, 1_uu, 2_uu
    };

    std::sort(indices.begin(), indices.end(), [&](usize a, usize b) {
        return (cam_pos - end_points[a]).length2() > (cam_pos - end_points[b]).length2();
    });




    auto compute_angle = [&](usize axis_index) {
        if(axis_index >= 3) {
            return 0.0f;
        }

        const math::Vec3 axis = basis[axis_index];
        const math::Vec3 u_axis = basis[(axis_index + 1) % 3];
        const math::Vec3 v_axis = basis[(axis_index + 2) % 3];

        const math::Vec3 projected_mouse = to_world_pos(mouse_pos);
        const math::Vec3 pt = intersect_ray_plane(cam_pos, (projected_mouse - cam_pos).normalized(), axis, -obj_pos.dot(axis));
        const math::Vec3 to_point = (pt - obj_pos).normalized();
        return std::acos(to_point.dot(u_axis)) * math::sign(to_point.dot(v_axis));
    };




    const usize segment_count = 64;
    const float inv_segment_count = 1.0f / segment_count;
    const float seg_ang_size = inv_segment_count * 2.0f * math::pi<float>;


    auto is_cam_side = [&](math::Vec3 world_pos) {
        const math::Vec3 local = world_pos - obj_pos;
        return local.normalized().dot((world_pos - cam_pos).normalized()) < 0.2f;
    };

    auto next_point = [&](usize axis, usize i) -> std::pair<math::Vec2, bool> {
        const math::Vec3 radial = basis[axis] * std::sin(i * seg_ang_size) + basis[(axis + 1) % 3] * std::cos(i * seg_ang_size);
        return {to_window_pos(obj_pos + radial * gizmo_radius * perspective), is_cam_side(obj_pos + radial)};
    };




    auto for_each_visible_segment = [&](usize axis, auto&& func) {
        math::Vec2 prev_point = next_point(axis, 0).first;
        for(usize i = 1; i != segment_count + 1; ++i) {
            const auto [next, visible] = next_point(axis, i);
            y_defer(prev_point = next);

            if(visible) {
                func(prev_point, next);
            }
        }
    };






    auto set_rotation = [transformable, selected, &world](const math::Quaternion<>& quat) {
        const math::Transform<> old_transform = transformable->transform();
        const auto [obj_pos, obj_rot, obj_scale] = old_transform.decompose();
        move_recursive(world, selected, old_transform, math::Transform<>(obj_pos, quat * obj_rot, obj_scale));
    };




    if(_is_dragging && _allow_dragging) {
        if(_rotation_axis != usize(-1)) {
            {
                const math::Vec3 u_axis = basis[(_rotation_axis + 1) % 3];
                const math::Vec3 v_axis = basis[(_rotation_axis + 2) % 3];
                const math::Vec3 d = u_axis * std::cos(_angle_offset) + v_axis * std::sin(_angle_offset);
                ImGui::GetWindowDrawList()->AddLine(to_window_pos(obj_pos), to_window_pos(obj_pos + d * perspective), gizmo_alpha | 0x00FFFFFF, gizmo_width * 0.5f);
            }

            const float angle = compute_angle(_rotation_axis);
            const math::Quaternion<> quat = math::Quaternion<>::from_axis_angle(basis[_rotation_axis], angle - _angle_offset);

            set_rotation(quat);
            _angle_offset = angle;
        }
    } else {
        _rotation_axis = usize(-1);
        for(usize axis : indices) {
            if(_rotation_axis != usize(-1)) {
                break;
            }

            for_each_visible_segment(axis, [&](math::Vec2 a, math::Vec2 b) {
                const math::Vec2 vec = b - a;
                if(vec.dot(b - mouse_pos) < 0.0f || vec.dot(a - mouse_pos) > 0.0f) {
                    return;
                }

                const math::Vec2 ortho = math::Vec2(-vec.y(), vec.x()).normalized();
                if(std::abs(ortho.dot(b - mouse_pos)) > 5.0f) {
                    return;
                }

                _rotation_axis = ((axis + 2) % 3);
                _angle_offset = compute_angle(_rotation_axis);
            });
        }

        _is_dragging = is_clicked() && (_rotation_axis != usize(-1));
    }

    for(usize axis : indices) {
        const usize rot_axis = (axis + 2) % 3;
        const bool is_current_axis = rot_axis == _rotation_axis;
        const u32 color = is_current_axis ? gizmo_hover_color : imgui::gizmo_color(rot_axis);

        for_each_visible_segment(axis, [&](math::Vec2 a, math::Vec2 b) {
            ImGui::GetWindowDrawList()->AddLine(a, b, gizmo_alpha | color, gizmo_width);
        });
    }
}






OrientationGizmo::OrientationGizmo(SceneView* view) : GizmoBase(view) {
}

void OrientationGizmo::draw() {
    y_profile();

    _is_dragging &= ImGui::IsMouseDown(ImGuiMouseButton_Left);

    const math::Vec2 offset = ImGui::GetWindowPos();
    const math::Vec2 viewport = ImGui::GetWindowSize();
    const math::Vec2 mouse_pos = ImGui::GetMousePos();

    const math::Vec2 center = offset + viewport - (orientation_gizmo_size * 2.0f);

    Camera& camera = _scene_view->camera();
    const auto view_proj = camera.view_proj_matrix();
    const auto view = camera.view_matrix();

    const float ratio = viewport.x() / viewport.y();

    std::array<i32, 4> axes = {
        -1, 0, 1, 2
    };

    const std::array axis_names = {
        "X", "Y", "Z"
    };


    std::sort(axes.begin(), axes.end(), [&](i32 a, i32 b) {
        math::Vec4 va, vb;
        if(a >= 0) {
            va[a] = 1.0f;
        }
        if(b >= 0) {
            vb[b] = 1.0f;
        }
        return (view * va).z() < (view * vb).z();
    });



    for(i32 i : axes) {
        if(i < 0) {
            ImGui::GetWindowDrawList()->AddCircleFilled(center, orientation_gizmo_width, 0xFFFFFFFF);
            continue;
        }

        const u32 color = orientation_gizmo_alpha | imgui::gizmo_color(i);

        math::Vec4 v;
        v[i] = orientation_gizmo_size + orientation_gizmo_end_point_width;
        const math::Vec2 axis = ((view_proj * v).to<2>() * 0.5f + 0.5f) * math::Vec2(ratio, 1.0f);
        const math::Vec2 end = center + axis;

        if(axis.length() > orientation_gizmo_end_point_width) {
            const math::Vec2 dir = axis.normalized();
            ImGui::GetWindowDrawList()->AddLine(center + dir * orientation_gizmo_width, end - dir * (orientation_gizmo_end_point_width - 1.0f), color, orientation_gizmo_width);
        }

        ImGui::GetWindowDrawList()->AddCircleFilled(end, orientation_gizmo_end_point_width, color);

        const math::Vec2 text_offset = math::Vec2(ImGui::CalcTextSize(axis_names[i]));
        ImGui::GetWindowDrawList()->AddText(end - text_offset * 0.5f, orientation_gizmo_alpha, axis_names[i]);
    }

    if(_allow_dragging) {
        const float full_radius = orientation_gizmo_size + orientation_gizmo_end_point_width * 2.0f;
        if(_is_dragging || (center - mouse_pos).length() < full_radius) {
            ImGui::GetWindowDrawList()->AddCircle(center, full_radius, orientation_gizmo_alpha | 0x00FFFFFF);
            if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                _is_dragging = true;
            }
        }
    }



    if(_is_dragging && _allow_dragging) {
        const math::Vec2 raw_mouse_delta = math::Vec2(ImGui::GetIO().MouseDelta);
        if(raw_mouse_delta.length() > 1.0f) {
            const math::Vec2 mouse_delta = raw_mouse_delta / (-0.5f * math::pi<float> * orientation_gizmo_size);
            const math::Vec3 cam_pos = camera.position();
            const math::Vec3 cam_fwd = camera.forward();
            // Hack to avoid the camera rolling
            const math::Vec3 cam_right = math::Vec3(camera.right().to<2>(), 0.0f).normalized();
            const math::Vec3 cam_up = cam_fwd.cross(cam_right).normalized();

            const math::Quaternion<> yaw = math::Quaternion<>::from_axis_angle(cam_up, mouse_delta.x());
            const math::Quaternion<> pitch = math::Quaternion<>::from_axis_angle(cam_right, -mouse_delta.y());

            _scene_view->camera().set_view(math::look_at(cam_pos, cam_pos + yaw(pitch(cam_fwd)), pitch(cam_up)));
        }
    }
}

}
