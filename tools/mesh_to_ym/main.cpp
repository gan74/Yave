#include <iostream>

#include "Mesh.h"

static int show_help() {
	return fatal("No help.");
}

static Vector<String> build_args(int argc, char** argv) {
	Vector<String> vec;
	for(int i = 1; i < argc; ++i) {
		vec << argv[i];
	}
	return vec;
}

int main(int argc, char** argv) {
	Vector<String> args = build_args(argc, argv);

	if(args.size() != 1) {
		return show_help();
	}

	auto scene = std::move(Scene::from_file(args[0]).expected("Unable to read file."));
	for(aiMesh* mesh : scene.meshes()) {
		Mesh::from_assimp(mesh).expected("Unable to build mesh").write(io::File::create(args[0] + ".ym").expected("Unable to open output file."));
	}

	return 0;
}
