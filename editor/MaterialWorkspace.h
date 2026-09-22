/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef EDITOR_MATERIALWORKSPACE_H
#define EDITOR_MATERIALWORKSPACE_H

#include "Workspace.h"

#include <yave/material/MaterialData.h>
#include <yave/assets/AssetId.h>

#include <variant>

namespace editor {

class MaterialWorkspace final : public Workspace {
    public:
        using EditMaterial = std::variant<
            MaterialData::MetallicRoughnessMaterialData,
            MaterialData::SpecularMaterialData
        >;

        MaterialWorkspace(AssetId id = {});
        ~MaterialWorkspace() override;

        void update() override;

        std::string_view name() const override;

        void save() override;
        void load() override;

        AssetId asset_id() const;
        EditMaterial& material();
        const EditMaterial& material() const;

        MaterialData material_data() const;

    private:
        void load_textures();
        void update_name();

        EditMaterial _material = MaterialData::MetallicRoughnessMaterialData{};
        AssetId _id;
        core::String _name = "Material";
};

}

#endif // EDITOR_MATERIALWORKSPACE_H
