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

#include "import.h"
#include "image_utils.h"

#include <yave/meshes/Vertex.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/MaterialData.h>
#include <yave/utils/FileSystemModel.h>

#include <y/io2/File.h>
#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


// -------------- TINYGLTF --------------
#ifdef Y_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif

#ifdef Y_MSVC
#pragma warning(push)
#pragma warning(disable:4018) // signed/unsigned mismatch
#pragma warning(disable:4267) // conversion from 'size_t' to 'int', possible loss of data
#endif


#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NOEXCEPTION
#include <external/tinygltf/tiny_gltf.h>


#ifdef Y_MSVC
#pragma warning(pop)
#endif

#ifdef Y_GNU
#pragma GCC diagnostic pop
#endif

// -------------- STB --------------
extern "C" {
#ifdef Y_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <external/stb/stb_image.h>

#ifdef Y_GNU
#pragma GCC diagnostic pop
#endif
}


namespace editor {
namespace import {

core::String clean_asset_name(const core::String& name) {
    if(name.is_empty()) {
        return "unamed";
    }

    usize begin = 0;
    for(usize i = 0; i != name.size(); ++i) {
        if(name[i] == '\\' || name[i] == '/') {
            begin = i + 1;
        }
    }

    usize len = 0;
    for(usize i = begin; i != name.size(); ++i) {
        if(name[i] == '.') {
            break;
        }
        ++len;
    }

    core::String n = name.sub_str(begin, len);
    for(char& c : n) {
        if(!std::isalnum(c)) {
            c = '_';
        }
    }

    return n.is_empty() ? core::String("unamed") : n;
}

core::Result<ImageData> import_image(const core::String& filename, ImageImportFlags flags) {
    if(auto file = io2::File::open(filename)) {
        core::Vector<u8> data;
        if(!file.unwrap().read_all(data)) {
            log_msg(fmt_c_str("Unable to read image \"{}\".", filename), Log::Error);
            return core::Err();
        }

        return import_image(data, flags);
    }

    log_msg(fmt_c_str("Unable to open image \"{}\".", filename), Log::Error);
    return core::Err();
}

core::Result<ImageData> import_image(core::Span<u8> image_data, ImageImportFlags flags) {
    y_profile();

    const usize req_components = 4;

    int width, height, bpp;
    u8* stbi_data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(image_data.data()), int(image_data.size()), &width, &height, &bpp, req_components);
    y_defer(stbi_image_free(stbi_data));

    if(!stbi_data || width < 1 || height < 1) {
        log_msg("Unable to load image", Log::Error);
        return core::Err();
    }

    ImageFormat format;
    if((flags & ImageImportFlags::ImportAsSRGB) == ImageImportFlags::ImportAsSRGB) {
        format = VK_FORMAT_R8G8B8A8_SRGB;
    } else {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    if(!format.is_valid()) {
        log_msg("Invalid image format", Log::Error);
        return core::Err();
    }

    ImageData img(math::Vec2ui(width, height), stbi_data, format);
    if((flags & ImageImportFlags::GenerateMipmaps) == ImageImportFlags::GenerateMipmaps) {
        img = compute_mipmaps(img);
    }

    if((flags & ImageImportFlags::Compress) == ImageImportFlags::Compress) {
        switch(bpp) {
            case 3:
                img = compress_bc1(img);
            break;

            /* case 1:
                img = compress_bc4(img);
            break; */

            default:
                log_msg("Compression is not supported for the given image format", Log::Error);
        }
    }

    return core::Ok(std::move(img));
}

core::String supported_image_extensions() {
    return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}







static usize component_count(int type) {
    switch(type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2: return 2;
        case TINYGLTF_TYPE_VEC3: return 3;
        case TINYGLTF_TYPE_VEC4: return 4;
        case TINYGLTF_TYPE_MAT2: return 4;
        case TINYGLTF_TYPE_MAT3: return 9;
        case TINYGLTF_TYPE_MAT4: return 16;
        default:
            y_throw_msg("Sparse accessors are not supported");
    }
}

static math::Transform<> build_base_change_transform() {
    const math::Vec3 forward = math::Vec3(1.0f, 0.0f, 0.0f);
    const math::Vec3 right = math::Vec3(0.0f, 0.0f, 1.0f);
    const math::Vec3 up = math::Vec3(0.0f, 1.0f, 0.0f);

    math::Transform<> transform;
    transform.set_basis(forward, right, up);
    return transform.inverse();
}

static const math::Transform<> base_change_transform = build_base_change_transform();

static math::Transform<> parse_node_transform_matrix(const tinygltf::Node& node) {
    y_debug_assert(node.matrix.size() == 16);

    math::Matrix4<> mat;
    std::transform(node.matrix.begin(), node.matrix.end(), mat.begin(), [](double f) {
        return float(f);
    });


    y_debug_assert(math::fully_finite(mat));
    y_debug_assert(mat[3][3] == 1.0f);
    return base_change_transform * mat;
}

static math::Transform<> parse_node_transform(const tinygltf::Node& node) {
    if(node.matrix.size() == 16) {
        return parse_node_transform_matrix(node);
    }

    y_debug_assert(node.matrix.empty());

    math::Vec3 translation;
    for(usize k = 0; k != node.translation.size(); ++k) {
        translation[k] = float(node.translation[k]);
    }

    math::Vec3 scale(1.0f, 1.0f, 1.0f);
    for(usize k = 0; k != node.scale.size(); ++k) {
        scale[k] = float(node.scale[k]);
    }

    math::Vec4 rotation(0.0f, 0.0f, 0.0f, 1.0f);
    for(usize k = 0; k != node.rotation.size(); ++k) {
        rotation[k] = float(node.rotation[k]);
    }

    rotation.to<3>() = base_change_transform.transform_direction(rotation.to<3>());
    translation = base_change_transform.transform_direction(translation);
    scale = base_change_transform.transform_direction(scale);

    const math::Transform<> tr = math::Transform<>(translation, rotation, scale);
    y_debug_assert(math::fully_finite(tr));
    y_debug_assert(tr[3][3] == 1.0f);
    return tr;
}




core::Result<ParsedScene> parse_scene(const core::String& filename) {
    y_profile();

    core::DebugTimer timer("Parsing gltf");

    ParsedScene scene;
    scene.filename = filename;
    scene.name = clean_asset_name(filename);
    scene.gltf = decltype(scene.gltf)(new tinygltf::Model(), [](tinygltf::Model* ptr) { delete ptr; });

    {
        y_profile_zone("glTF import");

        tinygltf::TinyGLTF ctx;

        // Set dummy loading function to avoid errors
        ctx.SetImageLoader([](tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void*) { return true; }, nullptr);

        const bool is_ascii = filename.ends_with(".gltf");
        const std::string file = filename.data();

        std::string err;
        std::string warn;
        const bool ok = is_ascii
                ? ctx.LoadASCIIFromFile(scene.gltf.get(), &err, &warn, file)
                : ctx.LoadBinaryFromFile(scene.gltf.get(), &err, &warn, file);

        if(!warn.empty()) {
            log_msg(warn, Log::Warning);
        }

        if(!err.empty()) {
            log_msg(err, Log::Error);
        }

        if(!ok) {
            return core::Err();
        }
    }


    scene.nodes.set_min_capacity(scene.gltf->nodes.size());
    for(const tinygltf::Node& gltf_node : scene.gltf->nodes) {
        auto& node = scene.nodes.emplace_back();

        node.name = gltf_node.name;
        node.transform = parse_node_transform(gltf_node);
        node.mesh_index = gltf_node.mesh;
        node.children = core::Vector<int>::from_range(gltf_node.children);

        if(node.name.is_empty()) {
            node.name = fmt_to_owned("node_{}", scene.nodes.size());
        }
    }


    {
        for(const auto& node : scene.nodes) {
            for(const int child_index : node.children) {
                scene.nodes[child_index].has_parent = true;
            }
        }

        for(usize i = 0; i != scene.nodes.size(); ++i) {
            if(!scene.nodes[i].has_parent) {
                scene.root_nodes << int(i);
            }
        }
    }

    return core::Ok(std::move(scene));
}

core::String supported_scene_extensions() {
    return "*.gltf;*.glb";
}

}
}

