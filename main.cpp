#include <iostream>
#include <y/io/File.h>
#include <y/io/BuffReader.h>
#include <y/io/Ref.h>

using namespace y;
using namespace io;

void print_file(ReaderRef file) {
	usize data_size = 10;
	char *data = new char[data_size];
	while(!file->at_end()) {
		usize read = file->read(data, data_size - 1);
		data[read] = 0;
		std::cout << data;
	}
	std::cout << std::endl;
}

int main(int, char **) {
	print_file(BuffReader(File::open("../y/core/vector.h")));
	return 0;
}
