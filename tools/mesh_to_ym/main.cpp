#include <iostream>

#include "Mesh.h"

#include <y/io/File.h>

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

	usize index = 0;
	for(aiMesh* ai : scene.meshes()) {
		auto mesh = Mesh::from_assimp(ai).expected("Unable to build mesh");
		auto file_name = (mesh.name().is_empty() ? args[0] : mesh.name()) + (index ? "_" + str(index) : "") + ".ym";
		mesh.write(io::File::create(file_name).expected("Unable to open output file."));
		log_msg(file_name + " exported");
		++index;
	}

	return 0;
}
