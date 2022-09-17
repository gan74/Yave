/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include "EntityView.h"

#include <editor/Settings.h>
#include <editor/Selection.h>
#include <editor/EditorWorld.h>
#include <editor/utils/ui.h>
#include <editor/widgets/Renamer.h>
#include <editor/widgets/AssetSelector.h>
#include <editor/widgets/DeletionDialog.h>
#include <editor/components/EditorComponent.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/scene/SceneView.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/utils/FileSystemModel.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/entities.h>
#include <yave/utils/color.h>

#include <y/math/random.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>

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
    if(TransformableComponent* transformable = world.component<TransformableComponent>(id)) {
        const float size = entity_radius(world, id).unwrap_or(5.0f);
        transformable->set_position(new_entity_pos(std::min(100.0f, size * 2.0f)));
    }
}

static void add_debug_lights() {
    y_profile();

    EditorWorld& world = current_world();

    const float spacing =  app_settings().debug.entity_spacing;
    const usize entity_count = app_settings().debug.entity_count;
    const usize side = usize(std::max(1.0, std::cbrt(entity_count)));

    const ecs::EntityId parent = world.create_collection_entity("Debug lights");
    world.component<EditorComponent>(parent)->set_hidden_in_editor(true);

    const math::Vec3 center = new_entity_pos(side * 1.5f);

    math::FastRandom rng;
    for(usize i = 0; i != entity_count; ++i) {
        const ecs::EntityId entity = world.create_entity<PointLightComponent>();

        world.set_entity_name(entity, "Debug light");
        world.set_parent(entity, parent);

        const math::Vec3 pos = center + math::Vec3(i / (side * side), (i / side) % side, i % side) - (side * 0.5f);
        world.component<TransformableComponent>(entity)->set_position(pos * spacing);
        world.component<EditorComponent>(entity)->set_hidden_in_editor(true);
        world.component<PointLightComponent>(entity)->radius() = spacing;
        world.component<PointLightComponent>(entity)->intensity() = spacing * spacing;
        world.component<PointLightComponent>(entity)->color() = identifying_color(rng());
    }
}

static void add_debug_entities() {
    y_profile();

    EditorWorld& world = current_world();

    const auto mesh = device_resources()[DeviceResources::CubeMesh];
    const auto material = device_resources()[DeviceResources::EmptyMaterial];

    const float spacing =  app_settings().debug.entity_spacing;
    const usize entity_count = app_settings().debug.entity_count;
    const usize side = usize(std::max(1.0, std::cbrt(entity_count)));

    const ecs::EntityId parent = world.create_collection_entity("Debug entities");
    world.component<EditorComponent>(parent)->set_hidden_in_editor(true);

    const math::Vec3 center = new_entity_pos(side * 1.5f);

    for(usize i = 0; i != entity_count; ++i) {
        const ecs::EntityId entity = world.create_entity<StaticMeshComponent>();

        world.set_entity_name(entity, "Debug entity");
        world.set_parent(entity, parent);

        const math::Vec3 pos = center + math::Vec3(i / (side * side), (i / side) % side, i % side) - (side * 0.5f);
        world.component<TransformableComponent>(entity)->set_position(pos * spacing);
        *world.component<StaticMeshComponent>(entity) = StaticMeshComponent(mesh, material);
        world.component<EditorComponent>(entity)->set_hidden_in_editor(true);
    }
}

static void add_prefab() {
    add_detached_widget<AssetSelector>(AssetType::Prefab, "Add prefab")->set_selected_callback(
        [](AssetId asset) {
            const ecs::EntityId id = current_world().add_prefab(asset);
            selection().set_selected(id);
            set_new_entity_pos(id);
            return id.is_valid();
        });
}

static void add_scene() {
    add_detached_widget<AssetSelector>(AssetType::Scene, "Add scene")->set_selected_callback(
        [](AssetId asset) {
            core::String name;
            if(const auto res = asset_store().name(asset)) {
                name = asset_store().filesystem()->filename(res.unwrap());
            }

            EditorWorld& world = current_world();
            world.add_scene(asset, world.create_collection_entity(name));
            return true;
        });
}


editor_action("Add debug lights", add_debug_lights)
editor_action("Add debug entities", add_debug_entities)
editor_action("Add prefab", add_prefab)
editor_action("Add scene", add_scene)


static void collect_all_descendants(core::Vector<ecs::EntityId>& descendants, EditorWorld& world, ecs::EntityId id) {
    if(EditorComponent* component = world.component<EditorComponent>(id)) {
        for(const ecs::EntityId child : component->children()) {
            descendants << child;
            collect_all_descendants(descendants, world, child);
        }
    }
}

static void populate_context_menu(EditorWorld& world, ecs::EntityId id = ecs::EntityId()) {
    if(EditorComponent* component = world.component<EditorComponent>(id)) {
        ImGui::Selectable(component->name().data(), false, ImGuiSelectableFlags_Disabled);

        ImGui::Separator();

        if(component->is_collection()) {
            if(ImGui::Selectable("Select immediate children")) {
                selection().set_selected(component->children());
            }
            if(ImGui::Selectable("Select all descendants")) {
                y_profile_zone("collect all descendants");
                auto descendants = core::vector_with_capacity<ecs::EntityId>(component->children().size() * 2);
                collect_all_descendants(descendants, world, id);
                selection().set_selected(descendants);
            }
            ImGui::Separator();
        }

        if(ImGui::Selectable("Rename")) {
            add_child_widget<Renamer>(component->name(), [=](std::string_view new_name) { component->set_name(new_name); return true; });
        }

        if(ImGui::Selectable(ICON_FA_TRASH " Delete")) {
            add_child_widget<DeletionDialog>(id);
        }

        ImGui::Separator();
    }

    if(ImGui::Selectable(ICON_FA_PLUS " New collection")) {
        world.set_parent(world.create_collection_entity("New collection"), id);
    }

    ecs::EntityId new_entity;
    if(ImGui::MenuItem(ICON_FA_PLUS " New empty entity")) {
        new_entity = world.create_entity();
    }

    ImGui::Separator();

    if(ImGui::MenuItem("Add prefab")) {
        add_prefab();
    }

    if(ImGui::MenuItem("Add scene")) {
        add_scene();
    }

    ImGui::Separator();

    if(ImGui::MenuItem(ICON_FA_LIGHTBULB " Add Point light")) {
        new_entity = world.create_named_entity("Point light", ecs::StaticArchetype<PointLightComponent>());
    }

    if(ImGui::MenuItem(ICON_FA_VIDEO " Add Spot light")) {
        new_entity = world.create_named_entity("Spot light", ecs::StaticArchetype<SpotLightComponent>());
    }

    if(new_entity.is_valid()) {
        world.set_parent(new_entity, id);
        selection().set_selected(new_entity);
        set_new_entity_pos(new_entity);
    }
}

static void display_entity(ecs::EntityId id, EditorWorld& world, ecs::SparseComponentSet<EditorComponent>& component_set, ecs::EntityId& context_menu_entity) {
    const float bottom = ImGui::GetScrollY() + ImGui::GetWindowHeight();
    const float top = ImGui::GetScrollY() - ImGui::GetTextLineHeightWithSpacing();
    const bool clipped = ImGui::GetCursorPosY() < top || ImGui::GetCursorPosY() > bottom;

    EditorComponent* component = component_set.try_get(id);

    const bool display_hidden = app_settings().debug.display_hidden_entities;
    if(!component || (!display_hidden && component->is_hidden_in_editor())) {
        return;
    }

    imgui::table_begin_next_row();

    const bool is_selected = !clipped && (context_menu_entity == id || selection().is_selected(id));
    const int flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | (is_selected ? ImGuiTreeNodeFlags_Selected : 0);

    auto update_selection = [&]() {
        if(ImGui::IsItemClicked()) {
            selection().add_or_remove(id, !ImGui::GetIO().KeyCtrl);
        }

        if(ImGui::IsItemClicked(1)) {
            context_menu_entity = id;
        }
    };

    if(component->is_collection()) {
        const char* full_display_name = clipped ? fmt_c_str("###%", id.as_u64()) : fmt_c_str(ICON_FA_BOX_OPEN " %###%", component->name(), id.as_u64());
        if(ImGui::TreeNodeEx(full_display_name, flags)) {
            update_selection();
            ImGui::Indent();
            for(ecs::EntityId id : component->children()) {
                display_entity(id, world, component_set, context_menu_entity);
            }
            ImGui::Unindent();
            ImGui::TreePop();
        } else {
            update_selection();
        }
    } else {
        const char* full_display_name;
        if(clipped) {
            full_display_name = fmt_c_str("###%", id.as_u64());
        } else {
            const std::string_view display_name = component->is_prefab() ? fmt("% (Prefab)", component->name()) : std::string_view(component->name());
            full_display_name = fmt_c_str("% %###%", world.entity_icon(id), display_name, id.as_u64());
        }

        if(ImGui::TreeNodeEx(full_display_name, flags | ImGuiTreeNodeFlags_Leaf)) {
            ImGui::TreePop();
        }
        update_selection();
    }
}


EntityView::EntityView() : Widget(ICON_FA_CUBES " Entities") {
}

void EntityView::on_gui() {
    EditorWorld& world = current_world();

    if(ImGui::Button(ICON_FA_PLUS, math::Vec2(24))) {
        ImGui::OpenPopup("##entitymenu");
    }

    if(ImGui::BeginPopup("##entitymenu")) {
        populate_context_menu(world);
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    ImGui::Text("%u entities", u32(world.components<EditorComponent>().size()));

    const ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;
    if(ImGui::BeginTable("##entities", 1, table_flags)) {
        y_profile_zone("fill entity panel");

        auto& editor_components = world.component_set<EditorComponent>();
        for(auto&& [id, comp] : editor_components) {
            if(!comp.has_parent()/* || !world.exists(comp.parent())*/) {
                display_entity(id, world, editor_components, _context_menu_entity);
            }
        }

        if(imgui::should_open_context_menu()) {
            ImGui::OpenPopup("##contextmenu");
            if(!ImGui::IsAnyItemHovered()) {
                _context_menu_entity = ecs::EntityId();
            }
        }

        if(ImGui::BeginPopup("##contextmenu")) {
            populate_context_menu(world, _context_menu_entity);
            ImGui::EndPopup();
        }
        ImGui::EndTable();
    }
}

}

