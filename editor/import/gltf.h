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
#ifndef EDITOR_IMPORT_GLTF_H
#define EDITOR_IMPORT_GLTF_H

#include <yave/meshes/Vertex.h>

#include <y/core/Result.h>
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/math/Transform.h>

namespace editor {
namespace import {
namespace gltf {

struct Primitive {
    core::Vector<FullVertex> vertices;
    core::Vector<IndexedTriangle> triangles;
    int material = -1;
};

struct Mesh {
    core::String name;
    core::Vector<Primitive> primitives;
};

enum class ColorSpace : u8 {
    Linear,
    sRGB,
};

struct Image {
    core::String name;

    core::String path;
    core::Vector<u8> data;

    ColorSpace color_space = ColorSpace::Linear;
    bool is_normal_map = false;
};

struct TextureRef {
    int image = -1;
    u32 uv_set = 0;

    bool is_valid() const {
        return image >= 0;
    }
};

enum class AlphaMode : u8 {
    Opaque,
    Mask,
    Blend,
};

struct Material {
    core::String name;

    TextureRef base_color;
    TextureRef normal;
    TextureRef emissive;
    TextureRef occlusion;
    TextureRef metallic_roughness;

    math::Vec4 base_color_factor = math::Vec4(1.0f);
    math::Vec3 emissive_factor;
    float metallic_factor = 1.0f;
    float roughness_factor = 1.0f;
    float normal_scale = 1.0f;
    float occlusion_strength = 1.0f;

    // KHR_materials_pbrSpecularGlossiness, base_color holds the diffuse texture and factor
    bool is_specular_glossiness = false;
    TextureRef specular_glossiness;
    math::Vec3 specular_factor = math::Vec3(1.0f);
    float glossiness_factor = 1.0f;

    AlphaMode alpha_mode = AlphaMode::Opaque;
    float alpha_cutoff = 0.5f;
    bool double_sided = false;
    bool unlit = false;
};

enum class LightType : u8 {
    Directional,
    Point,
    Spot,
};

struct Light {
    core::String name;

    LightType type = LightType::Point;
    math::Vec3 color = math::Vec3(1.0f);
    float intensity = 1.0f;
    float range = 0.0f; // 0 means infinite
    float inner_cone_angle = 0.0f;
    float outer_cone_angle = math::pi<float> / 4.0f;
};

struct Node {
    core::String name;
    math::Transform<> local_transform;

    int parent = -1;
    core::Vector<int> children;

    int mesh = -1;
    int light = -1;
};

struct Scene {
    core::String filename;

    core::Vector<Mesh> meshes;
    core::Vector<Material> materials;
    core::Vector<Image> images;
    core::Vector<Light> lights;
    core::Vector<Node> nodes;

    core::Vector<int> roots;
};

core::Result<Scene> load_gltf(const core::String& filename);

}
}
}

#endif // EDITOR_IMPORT_GLTF_H

