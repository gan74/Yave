/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <y/test/test.h>

#include <y/utils/log.h>

using namespace y;


#if 0

#include <y/serde3/archives.h>
#include <y/io2/File.h>
#include <y/utils/format.h>

#include <y/core/Result.h>

struct TestStruct {

	int a = 7;
	double b = 2.0;
	core::String floop = "ihandozlabndo";

	void print() {
		log_msg(fmt("% % %", a, b, floop));
	}

	y_serde3(b, a);
};


int main() {
	core::result::break_on_error = true;

	if(auto r0 = io2::File::open("test.bin")) {
		io2::File& file = r0.unwrap();

		TestStruct s;
		auto res = serde3::ReadableArchive(file).deserialize(s).unwrap();
		if(res == serde3::Success::Partial) {
			log_msg("Partial", Log::Warning);
		} else {
			log_msg("Ok");
		}
		s.print();
	} else {
		auto r1 =  io2::File::create("test.bin");
		io2::File& file = r1.unwrap();

		log_msg("File doesn't exists, creating", Log::Error);

		TestStruct s{ 19, 3.145f, "maap"};
		serde3::WritableArchive(file).serialize(s).unwrap();
		s.print();
	}
}

#else

int main() {

	const bool ok = test::run_tests();

	if(ok) {
		log_msg("All tests OK\n");
	} else {
		log_msg("Tests failed\n", Log::Error);
	}

	return ok ? 0 : 1;
}

#endif
