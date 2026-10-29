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

#include <array>

namespace editor {
namespace import {
namespace gltf {

// Strongest influences first, weights are not normalized (all zero if the vertex isn't weighted)
struct SkinInfluences {
    std::array<u32, SkinWeights::size> joints = {};     // Indices into Skin::joints
    std::array<float, SkinWeights::size> weights = {};
};

struct Primitive {
    core::Vector<FullVertex> vertices;
    core::Vector<IndexedTriangle> triangles;
    core::Vector<SkinInfluences> skin_influences; // Empty or one per vertex
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

// KHR_texture_transform
struct UvTransform {
    math::Vec2 offset;
    math::Vec2 scale = math::Vec2(1.0f);
    float rotation = 0.0f;

    bool is_identity() const {
        return offset.is_zero() && scale == math::Vec2(1.0f) && rotation == 0.0f;
    }

    bool operator==(const UvTransform&) const = default;

    math::Vec2 apply(const math::Vec2& uv) const {
        const math::Vec2 s = uv * scale;
        const float c = std::cos(rotation);
        const float r = std::sin(rotation);
        return offset + math::Vec2(c * s.x() + r * s.y(), c * s.y() - r * s.x());
    }
};

struct TextureRef {
    int image = -1;
    u32 uv_set = 0;
    UvTransform transform;

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
    TextureRef metallic_roughness;

    math::Vec4 base_color_factor = math::Vec4(1.0f);
    math::Vec3 emissive_factor;
    float metallic_factor = 1.0f;
    float roughness_factor = 1.0f;
    float normal_scale = 1.0f;

    // KHR_materials_pbrSpecularGlossiness
    bool is_specular_glossiness = false;
    TextureRef specular_glossiness;
    math::Vec3 specular_factor = math::Vec3(1.0f);
    float glossiness_factor = 1.0f;

    UvTransform uv_transform;

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

struct Skin {
    core::String name;

    core::Vector<int> joints;
    core::Vector<math::Transform<>> inverse_bind_matrices; // Empty if absent or invalid
};

enum class AnimationPath : u8 {
    Translation,
    Rotation,
    Scale,
};

enum class Interpolation : u8 {
    Linear,
    Step, // Cubic splines are decoded as linear
};

struct AnimationChannel {
    int node = -1;
    AnimationPath path = AnimationPath::Translation;
    Interpolation interpolation = Interpolation::Linear;

    core::Vector<float> times;
    core::Vector<math::Vec4> values; // One per key, xyz for translation and scale, xyzw quaternion for rotation
};

struct Animation {
    core::String name;
    core::Vector<AnimationChannel> channels;
};

struct Node {
    core::String name;
    math::Transform<> local_transform;

    int parent = -1;
    core::Vector<int> children;

    int mesh = -1;
    int light = -1;
    int skin = -1;
};

struct Scene {
    core::String filename;

    core::Vector<Mesh> meshes;
    core::Vector<Material> materials;
    core::Vector<Image> images;
    core::Vector<Light> lights;
    core::Vector<Skin> skins;
    core::Vector<Animation> animations;
    core::Vector<Node> nodes;

    core::Vector<int> roots;
};

core::Result<Scene> load_gltf(const core::String& filename);

}
}
}

#endif // EDITOR_IMPORT_GLTF_H

