#include <iostream>

#include <y/core/String.h>
#include <y/core/Vector.h>
#include <y/core/Chrono.h>
#include <y/core/Ptr.h>

#include <y/math/math.h>

#include <y/io/File.h>
#include <y/io/Ref.h>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace y;
using namespace y::core;

struct Vertex {
	math::Vec3 position;
	math::Vec3 normal;
	math::Vec3 tangent;
	math::Vec2 uv;
};

using IndexedTriangle = std::array<u32, 3>;


struct Mesh {
	Vector<Vertex> vertices;
	Vector<IndexedTriangle> triangles;

	float radius = 0.0f;


	Mesh(aiMesh* mesh) {
		if(mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
			fatal("Mesh is not triangulated.");
		}
		if(!mesh->HasNormals()) {
			fatal("Mesh is missing normals.");
		}
		if(!mesh->HasTangentsAndBitangents()) {
			fatal("Mesh is missing tangents.");
		}
		if(!mesh->HasTextureCoords(0)) {
			fatal("Mesh is missing uvs.");
		}

		usize vertex_count = mesh->mNumVertices;
		vertices.set_capacity(vertex_count);
		for(usize i = 0; i != vertex_count; ++i) {
			radius = std::max(radius, mesh->mVertices[i].Length());
			vertices << Vertex {
					{mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z},
					{mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z},
					{mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z},
					{mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}
				};
		}

		usize triangle_count = mesh->mNumFaces;
		triangles.set_capacity(triangle_count);

		for(usize i = 0; i != triangle_count; ++i) {
			triangles << IndexedTriangle{mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2]};
		}
	}
};

static Vector<String> build_args(int argc, char** argv) {
	Vector<String> vec;
	for(int i = 1; i < argc; ++i) {
		vec << argv[i];
	}
	return vec;
}

static int show_help() {
	std::cout << "DIS IS HELP" << std::endl;

	return 1;
}

static constexpr auto import_flags = aiProcess_Triangulate |
									 aiProcess_GenSmoothNormals |
									 aiProcess_CalcTangentSpace |
									 aiProcess_GenUVCoords |
									 aiProcess_ImproveCacheLocality |
									 aiProcess_OptimizeGraph |
									 aiProcess_OptimizeMeshes |
									 aiProcess_JoinIdenticalVertices;

static Vector<Mesh> process_file(const String& path) {
	Chrono timer;
	log_msg("Loading " + path + "...");

	Assimp::Importer importer;
	NotOwner<const aiScene*> scene = importer.ReadFile(path, import_flags);

	if(!scene) {
		fatal("Invalid file.");
	}
	if(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		log_msg("Incomplete scene.", Log::Error);
	}

	log_msg("Done in " + str(usize(timer.elapsed().to_millis())) + "ms", Log::Perf);

	log_msg(str(scene->mNumMeshes) + " meshes found");

	Vector<Mesh> meshes;
	for(usize i = 0; i != scene->mNumMeshes; ++i) {
		meshes << Mesh(scene->mMeshes[i]);
	}

	return meshes;
}


template<typename T>
static io::Writer::Result write_vector(io::WriterRef writer, const Vector<T>& vec) {
	auto res = writer->write_one(u32(vec.size()));
	if(res.is_error()) {
		return res;
	}
	return writer->write(vec.begin(), vec.size() * sizeof(T));
}

static void write_mesh(io::WriterRef writer, const Mesh& mesh) {
	auto error_msg = "Unable to write mesh.";

	u32 magic = 0x65766179;
	u32 type = 1;
	u32 version = 3;

	writer->write_one(magic).expected(error_msg);
	writer->write_one(type).expected(error_msg);
	writer->write_one(version).expected(error_msg);

	writer->write_one(mesh.radius).expected(error_msg);

	write_vector(writer, mesh.vertices).expected(error_msg);
	write_vector(writer, mesh.triangles).expected(error_msg);
}

int main(int argc, char** argv) {
	Vector<String> args = build_args(argc, argv);

	if(args.is_empty()) {
		return show_help();
	}

	for(const auto& arg : args) {
		for(const auto& mesh : process_file(arg)) {
			write_mesh(io::File::create(arg + ".ym").expected("Unable to create mesh file"), mesh);
			break;
		}
	}

	return 0;
}
