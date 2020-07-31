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
#ifndef YAVE_MATERIAL_SIMPLEMATERIALDATA_H
#define YAVE_MATERIAL_SIMPLEMATERIALDATA_H

#include <yave/graphics/descriptors/DescriptorSet.h>

#include <yave/assets/AssetPtr.h>
#include <yave/assets/AssetLoader.h>

namespace yave {

class SimpleMaterialData {

    public:
        enum Textures {
            Diffuse,
            Normal,
            Roughness,
            Metallic,
            Max
        };

        struct Contants {
            float roughness_mul = 1.0f;
            float metallic_mul = 0.0f;
        };

        Y_TODO(creating materials requires loading textures)

        static constexpr usize texture_count = usize(Textures::Max);

        SimpleMaterialData() = default;
        SimpleMaterialData(std::array<AssetPtr<Texture>, texture_count>&& textures);

        SimpleMaterialData& set_texture(Textures type, AssetPtr<Texture> tex);
        SimpleMaterialData& set_texture_reset_constants(Textures type, AssetPtr<Texture> tex);

        bool is_empty() const;

        const AssetPtr<Texture>& operator[](Textures tex) const;
        const std::array<AssetPtr<Texture>, texture_count>& textures() const;

        const Contants& constants() const { return _constants; }
        Contants& constants() { return _constants; }

        bool alpha_tested() const { return _alpha_tested; }
        bool& alpha_tested() { return _alpha_tested; }

        y_serde3(_textures, _constants, _alpha_tested)

    private:
        std::array<AssetId, texture_count> texture_ids() const;

        std::array<AssetPtr<Texture>, texture_count> _textures;
        Contants _constants;
        bool _alpha_tested = false;
};

}

#endif // YAVE_MATERIAL_BASICMATERIALDATA_H

