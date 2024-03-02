/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_MATERIAL_MATERIALDATA_H
#define YAVE_MATERIAL_MATERIALDATA_H

#include <yave/assets/AssetPtr.h>
#include <yave/graphics/images/Image.h>

namespace yave {

class MaterialData {
    public:
        struct CommonMaterialData {
            AssetPtr<Texture> diffuse;
            AssetPtr<Texture> normal;
            AssetPtr<Texture> emissive;

            math::Vec3 color_factor = math::Vec3(1.0f);
            math::Vec3 emissive_factor;

            bool alpha_tested = false;
            bool double_sided = false;
        };

        struct MetallicRoughnessMaterialData  {
            CommonMaterialData common;
            AssetPtr<Texture> metallic_roughness;
            float roughness_factor = 1.0f;
            float metallic_factor = 1.0f;
        };

        struct SpecularMaterialData {
            CommonMaterialData common;
            AssetPtr<Texture> specular;
            AssetPtr<Texture> specular_color;
            math::Vec3 specular_color_factor = math::Vec3(1.0f);
            float specular_factor = 1.0f;
        };

        enum class Type : u32 {
            Specular,
            MetallicRoughness,
        };

        static constexpr usize max_texture_count = 5;

        MaterialData() = default;
        MaterialData(MetallicRoughnessMaterialData data);
        MaterialData(SpecularMaterialData data);

        Type material_type() const;
        bool is_empty() const;

        core::Span<AssetPtr<Texture>> textures() const;

        math::Vec3 emissive_factor() const;
        math::Vec3 base_color_factor() const;
        math::Vec3 specular_color() const;

        float roughness_factor() const;
        float metallic_factor() const;
        float specular_factor() const;

        bool alpha_tested() const;
        bool double_sided() const;

        bool has_emissive() const;

        y_reflect(MaterialData,
              _type, _textures,
              _emissive_factor, _base_color_factor, _specular_color_factor,
              _roughness_factor, _metallic_factor, _specular_factor,
              _alpha_tested, _double_sided
        )

    private:
        MaterialData(Type type, CommonMaterialData data);

        std::array<AssetId, max_texture_count> texture_ids() const;

        Type _type = Type::MetallicRoughness;

        std::array<AssetPtr<Texture>, max_texture_count> _textures;

        math::Vec3 _emissive_factor;
        math::Vec3 _base_color_factor = math::Vec3(1.0f);
        math::Vec3 _specular_color_factor = math::Vec3(1.0f);

        float _roughness_factor = 1.0f;
        float _metallic_factor = 1.0f;
        float _specular_factor = 1.0f;

        bool _alpha_tested = false;
        bool _double_sided = false;
};

}

#endif // YAVE_MATERIAL_MATERIALDATA_H

