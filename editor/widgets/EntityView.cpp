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
#include "EntityView.h"

#include <editor/context/EditorContext.h>
#include <editor/components/EditorComponent.h>

#include <yave/ecs/ecs.h>
#include <yave/entities/entities.h>
#include <yave/utils/FileSystemModel.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>

#include <yave/assets/AssetLoader.h>

#include <editor/utils/ui.h>
#include <editor/utils/entities.h>
#include <editor/widgets/AssetSelector.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <imgui/yave_imgui.h>

namespace editor {


static auto all_components() {
    core::Vector<std::pair<core::String, ecs::ComponentRuntimeInfo>> components;

    for(const auto* poly_base = ecs::ComponentContainerBase::_y_serde3_poly_base.first; poly_base; poly_base = poly_base->next) {
        if(const auto container = poly_base->create()) {
            const ecs::ComponentRuntimeInfo info = container->runtime_info();
            components.emplace_back(clean_component_name(info.type_name), info);
        }
    }

    return components;
}


EntityView::EntityView(ContextPtr cptr) :
        Widget(ICON_FA_CUBES " Entities"),
        ContextLinked(cptr),
        _all_components(all_components()) {
}

void EntityView::paint_view() {
    const ecs::EntityWorld& world = context()->world();

    if(ImGui::BeginChild("##entities", ImVec2(), true)) {
        imgui::alternating_rows_background();
        for(ecs::EntityId id : world.ids()) {
            const EditorComponent* comp = world.component<EditorComponent>(id);
            if(!comp) {
                log_msg(fmt("Entity with id % is missing EditorComponent", id.index()), Log::Warning);
                continue;
            }

            const bool selected = context()->selection().selected_entity() == id;
            if(ImGui::Selectable(fmt_c_str("% %##%", entity_icon(world, id), comp->name(), id.index()), selected)) {
                 context()->selection().set_selected(id);
            }
            if(ImGui::IsItemHovered()) {
                _hovered = id;
            }
        }
    }
    ImGui::EndChild();
}

void EntityView::paint(CmdBufferRecorder&) {
    ecs::EntityWorld& world = context()->world();

    if(ImGui::Button(ICON_FA_PLUS, math::Vec2(24))) {
        ImGui::OpenPopup("Add entity");
    }


    if(ImGui::BeginPopup("Add entity")) {
        ecs::EntityId ent;
        if(ImGui::MenuItem(ICON_FA_PLUS " Add empty entity")) {
            ent = world.create_entity();
        }
        ImGui::Separator();

        if(ImGui::MenuItem("Add prefab")) {
            context()->ui_manager().add_widget<AssetSelector>(AssetType::Prefab)->set_selected_callback(
                [ctx = context()](AssetId asset) {
                    if(const auto prefab = ctx->loader().load_res<ecs::EntityPrefab>(asset)) {
                        const ecs::EntityId id = ctx->world().create_entity(*prefab.unwrap());
                        ctx->selection().set_selected(id);

                        const auto name = ctx->asset_store().name(asset);
                        if(name) {
                            set_entity_name(ctx->world(), id, fmt("% (Prefab)", ctx->asset_store().filesystem()->filename(name.unwrap())));
                        }
                    }
                    return true;
                });
        }


        ImGui::Separator();

        if(ImGui::MenuItem(ICON_FA_LIGHTBULB " Add Point light")) {
            ent = world.create_entity(PointLightArchetype());
            set_entity_name(world, ent, "Point light");
        }

        if(ImGui::MenuItem(ICON_FA_VIDEO " Add Spot light")) {
            ent = world.create_entity(SpotLightArchetype());
            set_entity_name(world, ent, "Spot light");
        }

        if(ent.is_valid()) {
            context()->selection().set_selected(ent);
        }

        y_debug_assert(!ent.is_valid() || world.has<EditorComponent>(ent));
        y_debug_assert(world.required_components().size() > 0);

        ImGui::EndPopup();
    }



    ImGui::Spacing();
    paint_view();



    if(_hovered.is_valid()) {
        if(ImGui::IsMouseReleased(1)) {
            ImGui::OpenPopup("##contextmenu");
        }

        if(ImGui::BeginPopup("##contextmenu")) {
            if(ImGui::BeginMenu(ICON_FA_PLUS " Add component")) {

                for(const auto& [name, info] : _all_components) {
                    const bool enabled = !name.is_empty() && !world.has(_hovered, info.type_id) && info.add_component;
                    if(ImGui::MenuItem(fmt_c_str(ICON_FA_PUZZLE_PIECE " %", name), nullptr, nullptr, enabled) && enabled) {
                        info.add_component(world, _hovered);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if(ImGui::Selectable(ICON_FA_TRASH " Delete")) {
                world.remove_entity(_hovered);
                // we don't unselect the ID to make sure that we can handle case where the id is invalid
            }

            if(ImGui::Selectable("Duplicate")) {
                const ecs::EntityPrefab prefab = world.create_prefab(_hovered);
                const ecs::EntityId copy = world.create_entity(prefab);
                if(EditorComponent* component = world.component<EditorComponent>(copy)) {
                    component->set_name(component->name() + " (Copy)");
                }
                context()->selection().set_selected(copy);
            }

            ImGui::EndPopup();
        } else {
            _hovered = ecs::EntityId();
        }
    }
}

}

