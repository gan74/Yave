#include <y/io/File.h>

#include "import.h"

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

	auto scene = import_scene(args[0]);

	usize index = 0;
	for(const auto& mesh : scene.meshes) {
		auto file_name = args[0] + (index ? "_" + str(index) : "") + ".ym";
		mesh.to_file(io::File::create(file_name).expected("Unable to open output file."));
		log_msg(file_name + " exported\n");
		++index;
	}

	for(const auto& anim : scene.animations) {
		auto file_name = args[0] + (index ? "_" + str(index) : "") + ".ya";
		anim.to_file(io::File::create(file_name).expected("Unable to open output file."));
		log_msg(file_name + " exported\n");
		++index;
	}

	return 0;
}
