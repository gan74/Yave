/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include <y/serde3/serde.h>
#include <y/serde3/archives.h>

#include <y/utils/name.h>
#include <y/utils/hash.h>

#include <y/core/String.h>
#include <y/io2/File.h>

using namespace y;
using namespace serde3;

y_test_func("Test test") {
	y_test_assert(true);
}




struct NestedStruct {
	float i = 3.14159f;

	y_serde3(i)
};

struct TestStruct {
	int x = 9;
	int y = 443;
	NestedStruct z;

	y_serde3(z, x, y)
};


int main() {
	/*{
		WritableArchive arc(std::move(io2::File::create("test.txt").unwrap()));
		TestStruct t{4, 5, {2.71727f}};
		arc.serialize(t).unwrap();
	}*/
	{
		ReadableArchive arc(std::move(io2::File::open("test.txt").unwrap()));
		TestStruct t;
		Success s = arc.deserialize(t).unwrap();

		log_msg(fmt("status = %", s == Success::Full ? "full" : "partial"));
		log_msg(fmt("{%, %, {%}}", t.x, t.y, t.z.i));
	}
	return 0;
}



