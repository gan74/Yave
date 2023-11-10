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

#include "Outliner.h"
#include "AssetSelector.h"
#include "DeletionDialog.h"
#include "Renamer.h"

#include <editor/Settings.h>
#include <editor/EditorWorld.h>
#include <editor/components/EditorComponent.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/scene/SceneView.h>

#include <editor/utils/assets.h>
#include <editor/utils/ui.h>

#include <y/math/random.h>

namespace editor {

static math::Vec3 new_entity_pos(float size) {
    const Camera& camera = scene_view().camera();
    return camera.position() + camera.forward() * size;
}

static void set_new_entity_pos(ecs::EntityId id) {
    if(!id.is_valid()) {
        return;
    }
    EditorWorld& world = current_world();
    if(TransformableComponent* transformable = world.component_mut<TransformableComponent>(id)) {
        const float size = transformable->global_aabb().radius();
        transformable->set_position(new_entity_pos(std::min(100.0f, size * 2.0f)));
    }
}

static void add_prefab() {
    add_detached_widget<AssetSelector>(AssetType::Prefab, "Add prefab")->set_selected_callback(
        [](AssetId asset) {
            const ecs::EntityId id = current_world().add_prefab(asset);
            current_world().set_selected(id);
            set_new_entity_pos(id);
            return id.is_valid();
        });
}




static void add_debug_lights() {
    y_profile();

    EditorWorld& world = current_world();

    const float spacing =  app_settings().debug.entity_spacing;
    const usize entity_count = app_settings().debug.entity_count;
    const usize side = usize(std::max(1.0, std::cbrt(entity_count)));

    const math::Vec3 center; //new_entity_pos(side * 1.5f);

    const ecs::EntityId parent = world.create_named_entity("Debug Lights");


    math::FastRandom rng;
    for(usize i = 0; i != entity_count; ++i) {
        const ecs::EntityId entity = world.create_named_entity(fmt("Debug light #{}", i));
        world.set_parent(entity, parent);

        const math::Vec3 pos = center + math::Vec3(i / (side * side), (i / side) % side, i % side) - (side * 0.5f);
        world.get_or_add_component<TransformableComponent>(entity)->set_position(pos * spacing);
        world.get_or_add_component<PointLightComponent>(entity)->range() = spacing;
        world.get_or_add_component<PointLightComponent>(entity)->intensity() = spacing * spacing;
        world.get_or_add_component<PointLightComponent>(entity)->color() = identifying_color(rng());
    }
}

editor_action("Add debug lights", add_debug_lights)


Outliner::Outliner() : Widget(ICON_FA_SITEMAP " Outliner") {
    _tag_buttons.emplace_back(ICON_FA_EYE, ecs::tags::hidden, false);
}

void Outliner::on_gui() {
    EditorWorld& world = current_world();

    if(ImGui::Button(ICON_FA_PLUS)) {
        ImGui::OpenPopup("##plusmenu");
    }

    if(ImGui::BeginPopup("##plusmenu")) {
        if(ImGui::MenuItem("Empty entity")) {
            current_world().set_selected(world.create_named_entity("New entity"));
        }

        ImGui::Separator();

        if(ImGui::MenuItem("Add Prefab")) {
            add_prefab();
        }

        ImGui::EndPopup();
    }


    if(ImGui::BeginTable("##entities", int(1 + _tag_buttons.size()))) {
        y_profile_zone("fill entity table");

        ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##tagbuttons",ImGuiTableColumnFlags_WidthFixed);

        for(const ecs::EntityId id : world.component_set<EditorComponent>().ids()) {
            if(world.has_parent(id)) {
                continue;
            }

            display_node(world, id);
        }

        ImGui::EndTable();
    }

    {
        ImGui::Indent();
        ImGui::Dummy(math::Vec2(ImGui::GetContentRegionAvail()) - math::Vec2(ImGui::GetStyle().FramePadding));
        make_drop_target(world, {});
        ImGui::Unindent();
    }
}

void Outliner::display_node(EditorWorld& world, ecs::EntityId id) {
    const EditorComponent* component = world.component<EditorComponent>(id);
    if(!component) {
        return;
    }


    auto display_tags = [&] {
        ImGui::TableNextColumn();

        for(const auto& [icon, tag, state] : _tag_buttons) {
            const bool tagged = world.has_tag(id, tag);
            if(tagged == state) {
                ImGui::TextUnformatted(icon);
            } else {
                ImGui::TextDisabled("%s", icon);
            }

            if(ImGui::IsItemClicked()) {
                if(tagged) {
                    world.remove_tag(id, tag);
                } else {
                    world.add_tag(id, tag);
                }
            }
        }
    };


    imgui::table_begin_next_row();

    const bool is_child_selected = world.is_parent(world.selected_entity(), id);
    const bool is_selected = world.is_selected(id);
    const bool has_children = world.has_children(id);

    const int flags =
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_OpenOnArrow |
        (is_selected || is_child_selected ? ImGuiTreeNodeFlags_Selected : 0) |
        (has_children ? 0 : ImGuiTreeNodeFlags_Leaf)
    ;

    const bool open = ImGui::TreeNodeEx(fmt_c_str("###{}", id.as_u64()), flags);

    if(ImGui::IsItemHovered()) {
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            _click_target = id;
        }
    } else if(_click_target == id) {
        _click_target = {};
    }

    if(_click_target == id) {
        const bool right_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

        if(ImGui::IsMouseReleased(ImGuiMouseButton_Left) || right_clicked) {
            world.toggle_selected(id, !ImGui::GetIO().KeyCtrl);
        }

        if(right_clicked) {
            _context_menu_target = id;
            ImGui::OpenPopup("##contextmenu");
        }

        if(!make_drop_target(world, id)) {
            if(ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload(imgui::drag_drop_entity_id, &id, sizeof(id));
                ImGui::EndDragDropSource();
            }
        }
    }

    ImGui::SameLine(0.0f, ImGui::GetStyle().FramePadding.x * 2.0f);
    imgui::text_icon(world.entity_icon(id));

    ImGui::SameLine();
    ImGui::TextUnformatted(component->name().data());

    display_tags();


    if(_context_menu_target.is_valid() && ImGui::BeginPopup("##contextmenu")) {
        ImGui::MenuItem(component->name().data(), "", false, false);
        ImGui::Separator();

        if(ImGui::MenuItem("Rename")) {
            add_child_widget<Renamer>(component->name(), [=](std::string_view name) {
                if(EditorComponent* comp = current_world().component_mut<EditorComponent>(_context_menu_target)) {
                    comp->set_name(name);
                }
                return true;
            });
        }

        if(ImGui::MenuItem(ICON_FA_TRASH " Delete")) {
            add_child_widget<DeletionDialog>(_context_menu_target);
        }

        ImGui::EndPopup();
    }


    if(open) {
        const auto children = core::Vector<ecs::EntityId>::from_range(world.children(id));
        for(const ecs::EntityId child : children) {
            display_node(world, child);
        }

        ImGui::TreePop();
    }
}

bool Outliner::make_drop_target(EditorWorld& world, ecs::EntityId id) {
    if(!ImGui::BeginDragDropTarget()) {
        return false;
    }

    if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(imgui::drag_drop_entity_id)) {
        if(const ecs::EntityId dragged = *static_cast<const ecs::EntityId*>(payload->Data); world.exists(dragged) && !world.is_parent(id, dragged)) {
            world.set_parent(dragged, id);
        }
    }
    ImGui::EndDragDropTarget();

    return true;
}


}

