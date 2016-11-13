#include <iostream>
#include <y/io/io.h>
#include <y/io/File.h>
#include <y/io/Ref.h>
#include <vector>

/*using namespace y;
using namespace y::io;

u8* decoded = nullptr;
u8* decoded_ptr = nullptr;
usize decoded_size = 0;

struct Header {
	u8 reserved		: 2;
	u8 content_chk	: 1;
	u8 sized		: 1;
	u8 block_chk	: 1;
	u8 indep		: 1;
	u8 version		: 2;
};

struct BlockData {
	u8 reserved		: 4;
	u8 max_size		: 3;
	u8 reserved_2	: 1;
};

struct Token {
	u8 match		: 4;
	u8 literals		: 4;
};

static_assert(sizeof(Header) == 1, "LZ4 header should be 1 byte");
static_assert(sizeof(BlockData) == 1, "LZ4 block data should be 1 byte");
static_assert(sizeof(Token) == 1, "LZ4 token should be 1 byte");

void cpy(usize dist, usize len) {
	for(usize i = 0; i < len; i += dist) {
		memmove(decoded_ptr + i, decoded_ptr - dist, len);
	}
	decoded_ptr += len;
}

void read_seq(ReaderRef file, u32& remaining) {
	Token tk{};
	remaining -= file->read(&tk, sizeof(tk));

	usize literals = tk.literals;
	if(literals == 15) {
		for(u8 b = 255; b == 255;) {
			remaining -= file->read(&b, sizeof(b));
			literals += b;
		}
	}

	remaining -= file->read(decoded_ptr, literals);
	decoded_ptr += literals;

	if(remaining) {

		u16 offset = 0;
		remaining -= file->read(&offset, sizeof(offset));

		usize match = tk.match;
		if(match == 15) {
			for(u8 b = 255; b == 255;) {
				remaining -= file->read(&b, sizeof(b));
				match += b;
			}
		}
		match += 4;
		cpy(offset, match);
	}
}

void read_block(ReaderRef file, bool checksum) {
	u32 size = 0;
	file->read(&size, sizeof(size));

	bool compressed = !(size >> 31);
	size = size & ~(1 << 31);

	std::cout << (compressed ? "compressed" : "uncompressed") << " (" << size / 1024 << "KB)\n";

	while(size) {
		usize s = size;
		read_seq(file, size);
		if(size > s) {
			fatal("Invalid LZ4 frame");
		}
	}

	if(checksum) {
		u32 chk = 0;
		file->read(&chk, sizeof(chk));
	}
}

void read_frame(ReaderRef file) {
	u32 magic = 0;
	file->read(&magic, sizeof(magic));
	if(magic != 0x184D2204) {
		fatal("Invalid LZ4 magic number");
	}

	Header header{};
	file->read(&header, sizeof(header));

	BlockData bd{};
	file->read(&bd, sizeof(bd));

	u64 size = 0;
	if(header.sized) {
		file->read(&size, sizeof(size));
	}

	u8 chksum = 0;
	file->read(&chksum, sizeof(chksum));

	if(header.version != 01 || header.reserved != 0 || bd.reserved != 0 || bd.reserved_2 != 0) {
		fatal("Invalid LZ4 header");
	}

	decoded_size = 256 << (2 * bd.max_size);
	//std::cout << usize(bd.max_size) << " -> " << decoded_size << std::endl;
	decoded_ptr = decoded = new u8[1024 * 1024];
	for(usize i = 0; i != 8; i++) {
		read_block(file, header.block_chk);
	}
}

int main(int, char** ) {
	system("cls");

	auto file = File::open("D:/Documents/projects/Miserables.txt.lz4");
	//std::cout << file.size() / 1024 << "KB" << std::endl;


	read_frame(file);
	std::cout << "size = " << (decoded_ptr - decoded) << std::endl;
	*decoded_ptr = 0;
	std::cout << decoded << std::endl;

	return 0;
}*/

#include <unordered_map>
#include <algorithm>

#include <y/utils.h>
#include <y/core/String.h>

using namespace y;
using namespace y::core;
using namespace y::io;

/*union A {};

struct X: A {
	int x;
};
*/

/*int main(int, char**) {
	if(String("a") > String("b")) {
		fatal("ozgao");
	}

	auto file = File::open("D:/Documents/projects/Miserables.txt");

	std::unordered_map<String, int> map;


	auto text = str(range(file.Reader::read_all()));
	std::cout << text.size() << std::endl;

	char vos[] = {'a', 'e', 'i', 'o', 'u', 'y'};

	{
		Chrono ch;

		String word;
		for(char c : text) {
			if(isspace(c) || (ispunct(c) && c != '\'')) {
				Vector<usize> vo;
				for(usize i = 0; i != word.size(); i++) {
					if(word[i] == 'a' ||
					   word[i] == 'e' ||
					   word[i] == 'i' ||
					   word[i] == 'o' ||
					   word[i] == 'u' ||
					   word[i] == 'y') {
						vo << i;
					}
				}
				if(vo.size() > 2) {
					auto index = vo[rand() % vo.size()];
					word[index] = vos[rand() % sizeof(vos)];
				}
				std::cout << word << " ";
				word = "";
			} else {
				word += str(char(tolower(c)));
			}
		}

		std::cout << ch.elapsed().to_millis() << "ms" << std::endl;
	}



	return 0;
}*/

#include <vector>
#include <iostream>


struct S {
	int s[128];

	S() {
		std::cout << "lele" << std::endl;
	}
};

template<typename T>
void test(T& e) {
	e();
}

using namespace y;

int main() {
	/*log(log::Error)("elelelelle").green("greeeeeeeen")("le");
	log("aaaaaaaaaaaaaa").blue("bluuuue");
	log("aaaaaaaaaaaaaa");*/

}






