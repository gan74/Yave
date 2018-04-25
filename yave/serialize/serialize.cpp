/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "serialize.h"

#include <yave/scene/Scene.h>
#include <yave/images/ImageData.h>
#include <yave/font/FontData.h>
#include <yave/meshes/MeshData.h>
#include <yave/animations/Animation.h>

#include <yave/material/Material.h>

#include <y/io/BuffReader.h>

#include <unordered_map>

namespace yave {

#define y_unwrap_to(var, expr)													\
	if(auto _unwrap_ = (expr); _unwrap_.is_ok()) { var = _unwrap_.unwrap(); }	\
	else { return core::Err(); }

#define y_unwrap(var, expr)														\
	std::remove_reference_t<decltype((expr).unwrap())> var;						\
	y_unwrap_to(var, expr)

#define y_try(expr) if(auto _unwrap_ = (expr); !_unwrap_.is_ok()) { return core::Err(); }

// -------------------------------------------------------- helpers --------------------------------------------------------

static core::Result<void> serialize(io::WriterRef writer, const core::String& str) {
	y_try(writer->write_one(u32(str.size())));
	y_try(writer->write(str.data(), str.size()));
	return core::Ok();
}

static core::Result<void> deserialize(io::ReaderRef reader, core::String& str) {
	y_unwrap(size, reader->read_one<u32>());
	str = core::String(nullptr, size);
	y_try(reader->read(str.data(), size));
	str[str.size()] = 0;

	return core::Ok();
}


// --------------------------------------------------------  animation  --------------------------------------------------------

core::Result<void> Animation::to_file(io::WriterRef writer) const {
	u32 channels = _channels.size();
	float duration = _duration;

	y_try(writer->write_one(fs::magic_number));
	y_try(writer->write_one(fs::animation_file_type));

	u32 version = 3;
	y_try(writer->write_one(version));

	y_try(writer->write_one(channels));
	y_try(writer->write_one(duration));


	for(const auto& ch : _channels) {
		y_try(writer->write_one(u32(ch.name().size())));
		y_try(writer->write(ch.name().begin(), ch.name().size()));

		y_try(writer->write_one(u32(ch.keys().size())));
		y_try(writer->write(ch.keys().begin(), ch.keys().size() * sizeof(AnimationChannel::BoneKey)));
	}

	return core::Ok();
}

core::Result<Animation> Animation::from_file(io::ReaderRef reader) {
	core::DebugTimer _("Animation::from_file()");
	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		u32 channels;
		float duration;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == fs::animation_file_type &&
				   version == 3 &&
				   channels > 0 &&
				   duration > 0.0f;
		}
	};

	y_unwrap(header, reader->read_one<Header>());
	if(!header.is_valid()) {
		return core::Err();
	}

	auto channels = core::vector_with_capacity<AnimationChannel>(header.channels);
	for(usize i = 0; i != header.channels; ++i) {
		core::String name;
		y_try(deserialize(reader, name));

		y_unwrap(key_count, reader->read_one<u32>());
		core::Vector<AnimationChannel::BoneKey> keys(key_count, AnimationChannel::BoneKey{});
		y_try(reader->read(keys.begin(), keys.size() * sizeof(AnimationChannel::BoneKey)));

		channels << AnimationChannel(name, std::move(keys));
	}

	return core::Ok(Animation(header.duration, std::move(channels)));
}

// --------------------------------------------------------  image  --------------------------------------------------------

core::Result<ImageData> ImageData::from_file(io::ReaderRef reader) {
	core::DebugTimer _("ImageData::from_file()");

	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		u32 width;
		u32 height;
		u32 layers;
		u32 mips;
		u32 format;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == fs::image_file_type &&
				   version == 3 &&
				   format > 0 &&
				   width != 0 &&
				   height != 0 &&
				   layers != 0 &&
				   mips != 0;
		}
	};

	y_unwrap(header, reader->read_one<Header>());
	if(!header.is_valid()) {
		return core::Err();
	}

	ImageData data;
	data._size = {header.width, header.height};
	data._layers = header.layers;
	data._mips = header.mips;
	data._format = ImageFormat(vk::Format(header.format));

	usize data_size = data.combined_byte_size();

	data._data = std::make_unique<u8[]>(data_size);

	y_try(reader->read(data._data.get(), data_size));

	return core::Ok(std::move(data));
}


// --------------------------------------------------------  font  --------------------------------------------------------

core::Result<FontData> FontData::from_file(io::ReaderRef reader) {
	core::DebugTimer _("FontData::from_file()");

	FontData font;

	if(auto img = ImageData::from_file(reader); img.is_ok()) {
		font._font_atlas = std::move(img.unwrap());
	} else {
		return core::Err();
	}

	struct Header {
		u32 magic;
		u32 type;
		u32 version;
		u32 char_count;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == fs::font_file_type &&
				   version == 1 &&
				   char_count > 0;
		}
	};

	y_unwrap(header, reader->read_one<Header>());
	if(!header.is_valid()) {
		return core::Err();
	}

	std::unique_ptr<Char[]> chars = std::make_unique<Char[]>(header.char_count);
	y_try(reader->read(chars.get(), header.char_count * sizeof(Char)));

	return core::Ok(std::move(font));
}


// -------------------------------------------------------- mesh --------------------------------------------------------

core::Result<void> MeshData::to_file(io::WriterRef writer) const {
	float radius = _radius;

	u32 bones = _skeleton ? _skeleton->bones.size() : 0;
	u32 vertices = _vertices.size();
	u32 triangles = _triangles.size();

	y_try(writer->write_one(fs::magic_number));
	y_try(writer->write_one(fs::mesh_file_type));

	u32 version = 5;
	y_try(writer->write_one(version));

	y_try(writer->write_one(radius));

	y_try(writer->write_one(bones));
	y_try(writer->write_one(vertices));
	y_try(writer->write_one(triangles));

	y_try(writer->write(_vertices.begin(), _vertices.size() * sizeof(Vertex)));
	y_try(writer->write(_triangles.begin(), _triangles.size() * sizeof(IndexedTriangle)));


	if(_skeleton) {
		y_try(writer->write(_skeleton->skin.begin(), _skeleton->skin.size() * sizeof(SkinWeights)));

		for(const auto& bone : _skeleton->bones) {
			y_try(writer->write_one(u32(bone.name.size())));
			y_try(writer->write(bone.name.data(), bone.name.size()));

			y_try(writer->write_one(bone.parent));
			y_try(writer->write_one(bone.local_transform));
		}
	}

	return core::Ok();
}

core::Result<MeshData> MeshData::from_file(io::ReaderRef reader) {
	core::DebugTimer _("MeshData::from_file()");

	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		float radius;

		u32 bones;
		u32 vertices;
		u32 triangles;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == fs::mesh_file_type &&
				   version == 5 &&
				   vertices != 0 &&
				   triangles != 0;
		}
	};


	y_unwrap(header, reader->read_one<Header>());
	if(!header.is_valid()) {
		return core::Err();
	}

	MeshData mesh;
	mesh._radius = header.radius;
	mesh._vertices = core::Vector<Vertex>(header.vertices, Vertex{});
	mesh._triangles = core::Vector<IndexedTriangle>(header.triangles, IndexedTriangle{});

	y_try(reader->read(mesh._vertices.begin(), header.vertices * sizeof(Vertex)));
	y_try(reader->read(mesh._triangles.begin(), header.triangles * sizeof(IndexedTriangle)));

	if(header.bones) {
		mesh._skeleton = std::make_unique<SkeletonData>();
		mesh._skeleton->skin = core::Vector<SkinWeights>(header.vertices, SkinWeights{});
		y_try(reader->read(mesh._skeleton->skin.begin(), header.vertices * sizeof(SkinWeights)));

		for(usize i = 0; i != header.bones; ++i) {
			core::String name;
			y_try(deserialize(reader, name));

			y_unwrap(parent, reader->read_one<u32>());
			y_unwrap(transform, reader->read_one<BoneTransform>());

			mesh._skeleton->bones << Bone{std::move(name), parent, transform};
		}
	}

	return core::Ok(std::move(mesh));
}


// -------------------------------------------------------- shaders --------------------------------------------------------

SpirVData SpirVData::from_file(io::ReaderRef reader) {
	core::Vector<u8> content;
	reader->read_all(content);
	core::Vector<u32> spriv32(content.size() / 4, 0);
	std::memcpy(spriv32.begin(), content.begin(), content.size());
	return SpirVData(spriv32);
}



// -------------------------------------------------------- scene --------------------------------------------------------

core::Result<void> Scene::to_file(io::WriterRef writer, const AssetLoader<StaticMesh>& mesh_loader) const {
	y_try(writer->write_one(fs::magic_number));
	y_try(writer->write_one(fs::scene_file_type));

	std::unordered_map<const StaticMesh*, u32> mesh_ids;
	for(const auto& asset : mesh_loader) {
		mesh_ids[asset.second.get()] = u32(mesh_ids.size());
	}

	u32 version = 1;
	y_try(writer->write_one(version));

	y_try(writer->write_one(u32(mesh_ids.size())));
	y_try(writer->write_one(u32(_statics.size())));
	y_try(writer->write_one(u32(_lights.size())));

	for(const auto& n : mesh_loader) {
		y_try(serialize(writer, n.first));
	}

	for(const auto& mesh : _statics) {
		if(auto it = mesh_ids.find(mesh->mesh().get()); it != mesh_ids.end()) {
			y_try(writer->write_one(it->second));
			y_try(writer->write_one(mesh->transform()));
		} else {
			return core::Err();
		}
	}

	for(const auto& light : _lights) {
		y_try(writer->write_one(*light));
	}

	for(const auto& renderable : _renderables) {
		log_msg("Can not serialize renderable of type \"" + type_name(*renderable) + "\"", Log::Warning);
	}

	return core::Ok();
}

core::Result<Scene> Scene::from_file(io::ReaderRef reader, AssetLoader<StaticMesh>& mesh_loader) {
	core::DebugTimer _("Scene::from_file()");

	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		u32 assets;
		u32 statics;
		u32 lights;

		bool is_valid() const {
			return magic == fs::magic_number &&
				   type == fs::scene_file_type &&
				   version == 1;
		}
	};

	y_unwrap(header, reader->read_one<Header>());
	if(!header.is_valid()) {
		return core::Err();
	}

	core::Vector<core::String> mesh_names;
	for(u32 i = 0; i != header.assets; ++i) {
		core::String name;
		y_try(deserialize(reader, name));
		mesh_names << name;
	}

	DevicePtr dptr = mesh_loader.device();
	auto material = make_asset<Material>(dptr, MaterialData()
			.set_frag_data(SpirVData::from_file(io::File::open("basic.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::from_file(io::File::open("basic.vert.spv").expected("Unable to load spirv file.")))
		);

	Scene scene;
	scene.static_meshes().set_min_capacity(header.statics);
	scene.lights().set_min_capacity(header.lights);

	{
		for(u32 i = 0; i != header.statics; ++i) {
			y_unwrap(mesh_id, reader->read_one<u32>());
			y_unwrap(mesh, mesh_loader.from_file(mesh_names[mesh_id]));

			auto inst = std::make_unique<StaticMeshInstance>(mesh, material);
			y_try(reader->read_one(inst->transform()));
			scene.static_meshes().emplace_back(std::move(inst));
		}
	}

	{
		for(u32 i = 0; i != header.lights; ++i) {
			// load as point, then read on top. Maybe change this ?
			auto light = std::make_unique<Light>(Light::Point);
			y_try(reader->read_one(*light));
			scene.lights().emplace_back(std::move(light));
		}
	}

	return core::Ok(std::move(scene));
}


#undef y_unwrap_to
#undef y_unwrap
#undef y_try

}
