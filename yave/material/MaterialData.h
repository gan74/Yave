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
#ifndef YAVE_MATERIAL_MATERIALDATA_H
#define YAVE_MATERIAL_MATERIALDATA_H

#include <yave/assets/AssetPtr.h>
#include <yave/graphics/images/Image.h>

namespace yave {

class MaterialData {

    public:
        enum Textures {
            Diffuse,
            Normal,
            Roughness,
            Metallic,
            Emissive,
            Max
        };

        static constexpr usize texture_count = usize(Textures::Max);

        MaterialData() = default;
        MaterialData(std::array<AssetPtr<Texture>, texture_count>&& textures);

        MaterialData& set_texture(Textures type, AssetPtr<Texture> tex);
        MaterialData& set_texture_reset_constants(Textures type, AssetPtr<Texture> tex);

        bool is_empty() const;

        const AssetPtr<Texture>& operator[](Textures tex) const;
        const std::array<AssetPtr<Texture>, texture_count>& textures() const;

        math::Vec3 emissive_mul() const;
        math::Vec3& emissive_mul();

        math::Vec3 base_color_mul() const;
        math::Vec3& base_color_mul();

        float roughness_mul() const;
        float& roughness_mul();

        float metallic_mul() const;
        float& metallic_mul();

        bool alpha_tested() const;
        bool& alpha_tested();

        Y_TODO(Not used!)
        bool double_sided() const;
        bool& double_sided();

        bool has_emissive() const;

        y_reflect(MaterialData,
              _textures,
              _emissive_mul, _base_color_mul,
              _roughness_mul, _metallic_mul,
              _alpha_tested, _double_sided
        )

    private:
        std::array<AssetId, texture_count> texture_ids() const;

        std::array<AssetPtr<Texture>, texture_count> _textures;

        math::Vec3 _emissive_mul;
        math::Vec3 _base_color_mul = math::Vec3(1.0f);
        float _roughness_mul = 1.0f;
        float _metallic_mul = 0.0f;

        bool _alpha_tested = false;
        bool _double_sided = false;
};

}

#endif // YAVE_MATERIAL_MATERIALDATA_H

