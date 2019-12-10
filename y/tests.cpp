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
#include <y/serde3/poly.h>

#include <y/utils/name.h>
#include <y/utils/hash.h>

#include <y/core/Chrono.h>
#include <y/core/String.h>
#include <y/core/FixedArray.h>
#include <y/io2/File.h>
#include <y/io2/Buffer.h>

#include <y/math/Vec.h>
#include <y/utils/log.h>
#include <y/utils/perf.h>

#include <thread>

using namespace y;

y_test_func("Test test") {
	y_test_assert(true);
}


void sleep() {
	y_profile();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}


int main() {
	perf::start_capture("perfdump.json");

	{
		y_profile_zone("a");
		sleep();
	}

	{
		y_profile_zone("b");
		sleep();
	}


	perf::end_capture();


	log_msg("Test OK");
	return 0;
}



